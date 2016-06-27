/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2013 Intel Corporation
 * Copyright (c) 2013-2015 Samsung Electronics Co., Ltd.
 *
 * Contact: Jihoon Kim <jihoon48.kim@samsung.com>, Li Zhang <li2012.zhang@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#define Uses_SCIM_PANEL_CLIENT
#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_PANEL_AGENT

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/times.h>
#include <pthread.h>
#include <langinfo.h>
#include <unistd.h>
#include <fcntl.h>

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <dlog.h>
#include <glib.h>

#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Wayland.h>
#include <vconf.h>
#include <vconf-keys.h>
#include <input-method-client-protocol.h>
#include <text-client-protocol.h>

#include "scim_private.h"
#include "scim.h"
#include "isf_wsc_context.h"
#include "isf_wsc_control_ui.h"

#include <linux/input.h>
#include <xkbcommon/xkbcommon.h>
#include <sys/mman.h>
#include "isf_debug.h"


#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG                                         "ISF_WAYLAND_MODULE"

using namespace scim;

struct _WSCContextISFImpl {
    WSCContextISF           *parent;
    Ecore_Wl_Window         *client_window;
    Ecore_IMF_Input_Mode     input_mode;
    WideString               preedit_string;
    AttributeList            preedit_attrlist;
    Ecore_IMF_Autocapital_Type autocapital_type;
    Ecore_IMF_Input_Hints    input_hint;
    Ecore_IMF_BiDi_Direction bidi_direction;
    void                    *imdata;
    int                      imdata_size;
    int                      preedit_caret;
    int                      cursor_x;
    int                      cursor_y;
    int                      cursor_top_y;
    int                      cursor_pos;
    bool                     use_preedit;
    bool                     is_on;
    bool                     preedit_started;
    bool                     preedit_updating;
    bool                     need_commit_preedit;
    int                      next_shift_status;
    int                      shift_mode_enabled;

    WSCContextISFImpl        *next;

    /* Constructor */
    _WSCContextISFImpl() : parent(NULL),
                           client_window(0),
                           input_mode(ECORE_IMF_INPUT_MODE_FULL),
                           autocapital_type(ECORE_IMF_AUTOCAPITAL_TYPE_SENTENCE),
                           input_hint(ECORE_IMF_INPUT_HINT_NONE),
                           bidi_direction(ECORE_IMF_BIDI_DIRECTION_NEUTRAL),
                           imdata(NULL),
                           imdata_size(0),
                           preedit_caret(0),
                           cursor_x(0),
                           cursor_y(0),
                           cursor_top_y(0),
                           cursor_pos(-1),
                           use_preedit(true),
                           is_on(true),
                           preedit_started(false),
                           preedit_updating(false),
                           need_commit_preedit(false),
                           next_shift_status(0),
                           shift_mode_enabled(0),
                           next(NULL)
    {
    }
};

typedef enum {
    INPUT_LANG_URDU,
    INPUT_LANG_HINDI,
    INPUT_LANG_BENGALI_IN,
    INPUT_LANG_BENGALI_BD,
    INPUT_LANG_ASSAMESE,
    INPUT_LANG_PUNJABI,
    INPUT_LANG_NEPALI,
    INPUT_LANG_ORIYA,
    INPUT_LANG_MAITHILI,
    INPUT_LANG_ARMENIAN,
    INPUT_LANG_CN,
    INPUT_LANG_CN_HK,
    INPUT_LANG_CN_TW,
    INPUT_LANG_JAPANESE,
    INPUT_LANG_KHMER,
    INPUT_LANG_BURMESE,
    INPUT_LANG_OTHER
} Input_Language;

struct __Punctuations {
    const char *code;
    Input_Language lang;
    wchar_t punc_code;
};

static __Punctuations __punctuations [] = {
    { "ur_PK",  INPUT_LANG_URDU,        0x06D4 },
    { "hi_IN",  INPUT_LANG_HINDI,       0x0964 },
    { "bn_IN",  INPUT_LANG_BENGALI_IN,  0x0964 },
    { "bn_BD",  INPUT_LANG_BENGALI_BD,  0x0964 },
    { "as_IN",  INPUT_LANG_ASSAMESE,    0x0964 },
    { "pa_IN",  INPUT_LANG_PUNJABI,     0x0964 },
    { "ne_NP",  INPUT_LANG_NEPALI,      0x0964 },
    { "or_IN",  INPUT_LANG_ORIYA,       0x0964 },
    { "mai_IN", INPUT_LANG_MAITHILI,    0x0964 },
    { "hy_AM",  INPUT_LANG_ARMENIAN,    0x0589 },
    { "zh_CN",  INPUT_LANG_CN,          0x3002 },
    { "zh_HK",  INPUT_LANG_CN_HK,       0x3002 },
    { "zh_TW",  INPUT_LANG_CN_TW,       0x3002 },
    { "ja_JP",  INPUT_LANG_JAPANESE,    0x3002 },
    { "km_KH",  INPUT_LANG_KHMER,       0x17D4 },
};

/* private functions */

static void     panel_slot_update_preedit_caret         (int                     context,
                                                         int                     caret);
static void     panel_slot_process_key_event            (int                     context,
                                                         const KeyEvent         &key);
static void     panel_slot_commit_string                (int                     context,
                                                         const WideString       &wstr);
static void     panel_slot_forward_key_event            (int                     context,
                                                         const KeyEvent         &key);
static void     _show_preedit_string          (int                     context);
static void     _update_preedit_string        (int                     context,
                                                         const WideString       &str,
                                                         const AttributeList    &attrs,
                                                         int               caret);
static void     panel_req_update_bidi_direction         (WSCContextISF     *ic, int direction);

/* Panel iochannel handler*/
static void     panel_initialize                        (void);
static void     panel_finalize                          (void);


/* utility functions */

static bool     filter_keys                             (const char *keyname,
                                                         const char *config_path);
static void     set_ic_capabilities                     (WSCContextISF     *ic);

static void     initialize                              (void);
static void     finalize                                (void);

static void     send_wl_key_event                       (WSCContextISF *ic, const KeyEvent &key, bool fake);
static void     _hide_preedit_string                    (int context, bool update_preedit);

/* Local variables declaration */
static String                                           _language;
static WSCContextISFImpl                               *_used_ic_impl_list          = 0;
static WSCContextISFImpl                               *_free_ic_impl_list          = 0;
static WSCContextISF                                   *_ic_list                    = 0;

static KeyboardLayout                                   _keyboard_layout            = SCIM_KEYBOARD_Default;
static int                                              _valid_key_mask             = SCIM_KEY_AllMasks;

static ConfigPointer                                    _config;

static WSCContextISF                                   *_focused_ic                 = 0;

static bool                                             _scim_initialized           = false;

static int                                              _panel_client_id            = 0;
static uint32                                           _active_helper_option       = 0;

static bool                                             _on_the_spot                = true;
static bool                                             _change_keyboard_mode_by_focus_move = false;
static bool                                             _support_hw_keyboard_mode   = false;
static double                                           space_key_time              = 0.0;

static Eina_Bool                                        autoperiod_allow            = EINA_FALSE;
static Eina_Bool                                        autocap_allow               = EINA_FALSE;

static bool                                             _x_key_event_is_valid       = false;

static Input_Language                                   input_lang                  = INPUT_LANG_OTHER;

//FIXME: remove this definitions
#define SHIFT_MODE_OFF  0xffe1
#define SHIFT_MODE_ON   0xffe2
#define SHIFT_MODE_LOCK 0xffe6
#define SHIFT_MODE_ENABLE 0x9fe7
#define SHIFT_MODE_DISABLE 0x9fe8

#define WAYLAND_MODULE_CLIENT_ID (0)

//////////////////////////////wayland_panel_agent_module begin//////////////////////////////////////////////////

#define scim_module_init wayland_LTX_scim_module_init
#define scim_module_exit wayland_LTX_scim_module_exit
#define scim_panel_agent_module_init wayland_LTX_scim_panel_agent_module_init
#define scim_panel_agent_module_get_instance wayland_LTX_scim_panel_agent_module_get_instance

static struct weescim _wsc                                  = {0};

InfoManager* g_info_manager = NULL;


/////////////////////////////////////////////////////////////////////////////
// Implementation of Wayland Input Method functions.
/////////////////////////////////////////////////////////////////////////////

static void
_wsc_im_ctx_reset(void *data, struct wl_input_method_context *im_ctx)
{
    WSCContextISF *context_scim = (WSCContextISF*)data;
    LOGD ("");
    if (context_scim && context_scim->impl && context_scim == _focused_ic) {
        g_info_manager->socket_reset_input_context (WAYLAND_MODULE_CLIENT_ID, context_scim->id);

        if (context_scim->impl->need_commit_preedit) {
            _hide_preedit_string (context_scim->id, false);
            wsc_context_commit_preedit_string (context_scim);
        }
    }
}

static void
_wsc_im_ctx_content_type(void *data, struct wl_input_method_context *im_ctx, uint32_t hint, uint32_t purpose)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    if (!wsc_ctx) return;

    LOGD ("im_context = %p hint = %d purpose = %d\n", im_ctx, hint, purpose);

    if (!wsc_ctx->context_changed) return;

    wsc_ctx->content_hint = hint;
    wsc_ctx->content_purpose = purpose;

    isf_wsc_context_input_panel_layout_set (wsc_ctx,
                                            wsc_context_input_panel_layout_get (wsc_ctx));

    isf_wsc_context_autocapital_type_set (wsc_ctx, wsc_context_autocapital_type_get(wsc_ctx));

    isf_wsc_context_input_panel_language_set (wsc_ctx, wsc_context_input_panel_language_get(wsc_ctx));

    caps_mode_check (wsc_ctx, EINA_TRUE, EINA_TRUE);

    wsc_ctx->context_changed = EINA_FALSE;
}

static void
_wsc_im_ctx_invoke_action(void *data, struct wl_input_method_context *im_ctx, uint32_t button, uint32_t index)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    if (!wsc_ctx) return;

    LOGD ("invoke action. button : %d\n", button);

    if (button != BTN_LEFT)
        return;

    wsc_context_send_preedit_string (wsc_ctx);
}

static void
_wsc_im_ctx_commit_state(void *data, struct wl_input_method_context *im_ctx, uint32_t serial)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    if (!wsc_ctx) return;

    wsc_ctx->serial = serial;

    if (wsc_ctx->language)
        wl_input_method_context_language (im_ctx, wsc_ctx->serial, wsc_ctx->language);
}

static void
_wsc_im_ctx_preferred_language(void *data, struct wl_input_method_context *im_ctx, const char *language)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    if (!wsc_ctx) return;

    if (language && wsc_ctx->language && !strcmp (language, wsc_ctx->language))
        return;

    if (wsc_ctx->language) {
        free (wsc_ctx->language);
        wsc_ctx->language = NULL;
    }

    if (language) {
        wsc_ctx->language = strdup (language);
        LOGD ("Language changed, new: '%s'\n", language);
    }
}

static void
_wsc_im_ctx_return_key_type(void *data, struct wl_input_method_context *im_ctx, uint32_t return_key_type)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;

    LOGD ("im_context = %p return key type = %d\n", im_ctx, return_key_type);
    if (!wsc_ctx) return;

    if (wsc_ctx->return_key_type != return_key_type) {
        wsc_ctx->return_key_type = return_key_type;
        isf_wsc_context_input_panel_return_key_type_set (wsc_ctx, (Ecore_IMF_Input_Panel_Return_Key_Type)wsc_ctx->return_key_type);
    }
}

static void
_wsc_im_ctx_return_key_disabled(void *data, struct wl_input_method_context *im_ctx, uint32_t disabled)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    Eina_Bool return_key_disabled = !!disabled;

    LOGD ("im_context = %p return key disabled = %d\n", im_ctx, return_key_disabled);
    if (!wsc_ctx) return;

    if (wsc_ctx->return_key_disabled != return_key_disabled) {
        wsc_ctx->return_key_disabled = return_key_disabled;
        isf_wsc_context_input_panel_return_key_disabled_set (wsc_ctx, wsc_ctx->return_key_disabled);
    }
}

static void
_wsc_im_ctx_input_panel_data(void *data, struct wl_input_method_context *im_ctx, const char *input_panel_data, uint32_t input_panel_data_length)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    LOGD ("im_context = %p input panel data = %s len = %d\n", im_ctx, input_panel_data, input_panel_data_length);
    if (!wsc_ctx) return;

    isf_wsc_context_input_panel_imdata_set (wsc_ctx, (void *)input_panel_data, input_panel_data_length);
}

static void
_wsc_im_ctx_bidi_direction(void *data, struct wl_input_method_context *im_ctx, uint32_t bidi_direction)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;

    LOGD ("im_context = %p bidi_direction = %d\n", im_ctx, bidi_direction);
    if (!wsc_ctx) return;

    if (wsc_ctx->bidi_direction != bidi_direction) {
        wsc_ctx->bidi_direction = bidi_direction;

        isf_wsc_context_bidi_direction_set (wsc_ctx, (Ecore_IMF_BiDi_Direction)wsc_ctx->bidi_direction);
    }
}

static void
_wsc_im_ctx_cursor_position(void *data, struct wl_input_method_context *im_ctx, uint32_t cursor_pos)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;

    LOGD ("im_context = %p cursor_pos = %d\n", im_ctx, cursor_pos);
    if (!wsc_ctx || !wsc_ctx->impl) return;
    wsc_ctx->impl->cursor_pos = cursor_pos;
    wsc_ctx->surrounding_cursor = cursor_pos;
    caps_mode_check (wsc_ctx, EINA_FALSE, EINA_TRUE);
    g_info_manager->socket_update_cursor_position (cursor_pos);
}

static void
_wsc_im_ctx_process_input_device_event(void *data, struct wl_input_method_context *im_ctx, uint32_t type, const char *input_data, uint32_t input_data_len)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;

    LOGD("im_context = %p type = %d, data = (%p) %d\n", im_ctx, type, input_data, input_data_len);
    if (!wsc_ctx) return;

    isf_wsc_context_process_input_device_event(wsc_ctx, type, input_data, input_data_len);
}

static const struct wl_input_method_context_listener wsc_im_context_listener = {
     _wsc_im_ctx_reset,
     _wsc_im_ctx_content_type,
     _wsc_im_ctx_invoke_action,
     _wsc_im_ctx_commit_state,
     _wsc_im_ctx_preferred_language,
     _wsc_im_ctx_return_key_type,
     _wsc_im_ctx_return_key_disabled,
     _wsc_im_ctx_input_panel_data,
     _wsc_im_ctx_bidi_direction,
     _wsc_im_ctx_cursor_position,
     _wsc_im_ctx_process_input_device_event
};

static void
_init_keysym2keycode(WSCContextISF *wsc_ctx)
{
    uint32_t i = 0;
    uint32_t code;
    uint32_t num_syms;
    const xkb_keysym_t *syms;

    if (!wsc_ctx || !wsc_ctx->state)
        return;

    for (i = 0; i < 256; i++) {
        code = i + 8;
        num_syms = xkb_key_get_syms(wsc_ctx->state, code, &syms);

        if (num_syms == 1)
            wsc_ctx->_keysym2keycode[syms[0]] = i;
    }
}

static void
_fini_keysym2keycode(WSCContextISF *wsc_ctx)
{
    wsc_ctx->_keysym2keycode.clear();
}

static void
_wsc_im_keyboard_keymap(void *data,
        struct wl_keyboard *wl_keyboard,
        uint32_t format,
        int32_t fd,
        uint32_t size)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    char *map_str;

    if (!wsc_ctx) return;

    _fini_keysym2keycode(wsc_ctx);

    if (wsc_ctx->state) {
        xkb_state_unref(wsc_ctx->state);
        wsc_ctx->state = NULL;
    }

    if (wsc_ctx->keymap) {
        xkb_map_unref(wsc_ctx->keymap);
        wsc_ctx->keymap = NULL;
    }

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    map_str = (char*)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (map_str == MAP_FAILED) {
        close(fd);
        return;
    }

    wsc_ctx->keymap =
        xkb_map_new_from_string(wsc_ctx->xkb_context,
                map_str,
                XKB_KEYMAP_FORMAT_TEXT_V1,
                (xkb_keymap_compile_flags)0);

    munmap(map_str, size);
    close(fd);

    if (!wsc_ctx->keymap) {
        LOGW ("failed to compile keymap\n");
        return;
    }

    wsc_ctx->state = xkb_state_new(wsc_ctx->keymap);
    if (!wsc_ctx->state) {
        LOGW ("failed to create XKB state\n");
        xkb_map_unref(wsc_ctx->keymap);
        return;
    }

    wsc_ctx->control_mask =
        1 << xkb_map_mod_get_index(wsc_ctx->keymap, "Control");
    wsc_ctx->alt_mask =
        1 << xkb_map_mod_get_index(wsc_ctx->keymap, "Mod1");
    wsc_ctx->shift_mask =
        1 << xkb_map_mod_get_index(wsc_ctx->keymap, "Shift");

    LOGD ("create _keysym2keycode\n");
    _init_keysym2keycode(wsc_ctx);
}

static void
_wsc_im_keyboard_key(void *data,
        struct wl_keyboard *wl_keyboard,
        uint32_t serial,
        uint32_t time,
        uint32_t key,
        uint32_t state_w)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    uint32_t code;
    uint32_t num_syms;
    const xkb_keysym_t *syms;
    xkb_keysym_t sym;
    char keyname[64] = {0};
    enum wl_keyboard_key_state state = (wl_keyboard_key_state)state_w;

    if (!wsc_ctx || !wsc_ctx->state)
        return;

    code = key + 8;
    num_syms = xkb_key_get_syms(wsc_ctx->state, code, &syms);

    sym = XKB_KEY_NoSymbol;
    if (num_syms == 1)
    {
        sym = syms[0];
        xkb_keysym_get_name(sym, keyname, 64);
    }

    if (wsc_ctx->key_handler)
        (*wsc_ctx->key_handler)(wsc_ctx, serial, time, code, sym, keyname,
                state);
}

static void
_wsc_im_keyboard_modifiers(void *data,
        struct wl_keyboard *wl_keyboard,
        uint32_t serial,
        uint32_t mods_depressed,
        uint32_t mods_latched,
        uint32_t mods_locked,
        uint32_t group)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    if (!wsc_ctx || !wsc_ctx->state)
        return;

    struct wl_input_method_context *context = wsc_ctx->im_ctx;
    xkb_mod_mask_t mask;

    xkb_state_update_mask(wsc_ctx->state, mods_depressed,
            mods_latched, mods_locked, 0, 0, group);
    mask = xkb_state_serialize_mods(wsc_ctx->state,
            (xkb_state_component)(XKB_STATE_DEPRESSED | XKB_STATE_LATCHED));

    wsc_ctx->modifiers = 0;
    if (mask & wsc_ctx->control_mask)
        wsc_ctx->modifiers |= SCIM_KEY_ControlMask;
    if (mask & wsc_ctx->alt_mask)
        wsc_ctx->modifiers |= SCIM_KEY_AltMask;
    if (mask & wsc_ctx->shift_mask)
        wsc_ctx->modifiers |= SCIM_KEY_ShiftMask;

    wl_input_method_context_modifiers(context, serial,
            mods_depressed, mods_depressed,
            mods_latched, group);
}

static const struct wl_keyboard_listener wsc_im_keyboard_listener = {
    _wsc_im_keyboard_keymap,
    NULL, /* enter */
    NULL, /* leave */
    _wsc_im_keyboard_key,
    _wsc_im_keyboard_modifiers
};

static void
_wsc_im_activate(void *data, struct wl_input_method *input_method, struct wl_input_method_context *im_ctx, uint32_t text_input_id)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc) return;

    WSCContextISF *wsc_ctx = new WSCContextISF;
    if (!wsc_ctx) {
        return;
    }

    wsc_ctx->xkb_context = xkb_context_new((xkb_context_flags)0);
    if (wsc_ctx->xkb_context == NULL) {
        LOGW ("Failed to create XKB context\n");
        delete wsc_ctx;
        return;
    }

    wsc_ctx->id = text_input_id;
    wsc->wsc_ctx = wsc_ctx;
    wsc_ctx->ctx = wsc;
    wsc_ctx->state = NULL;
    wsc_ctx->keymap = NULL;
    wsc_ctx->modifiers = 0;
    wsc_ctx->surrounding_text = NULL;
    wsc_ctx->key_handler = isf_wsc_context_filter_key_event;

    get_language(&wsc_ctx->language);

    wsc_ctx->preedit_str = strdup ("");
    wsc_ctx->content_hint = WL_TEXT_INPUT_CONTENT_HINT_NONE;
    wsc_ctx->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_NORMAL;

    wsc_ctx->im_ctx = im_ctx;
    wl_input_method_context_add_listener (im_ctx, &wsc_im_context_listener, wsc_ctx);

    wsc_ctx->keyboard = wl_input_method_context_grab_keyboard(im_ctx);
    if (wsc_ctx->keyboard)
        wl_keyboard_add_listener(wsc_ctx->keyboard, &wsc_im_keyboard_listener, wsc_ctx);

    if (wsc_ctx->language)
        wl_input_method_context_language (im_ctx, wsc_ctx->serial, wsc_ctx->language);

    isf_wsc_context_add (wsc_ctx);

    wsc_ctx->context_changed = EINA_TRUE;

    isf_wsc_context_focus_in (wsc_ctx);

    int len = 0;
    char *imdata = NULL;
    isf_wsc_context_input_panel_imdata_get (wsc_ctx, (void**)&imdata, &len);
    LOGD ("Get imdata:%s, length:%d\n", imdata, len);
    if (imdata && len)
        wl_input_method_context_update_input_panel_data (im_ctx, wsc_ctx->serial, imdata, len);
    if (imdata)
        delete[] imdata;
}

static void
_wsc_im_deactivate(void *data, struct wl_input_method *input_method, struct wl_input_method_context *im_ctx)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc || !wsc->wsc_ctx) return;
    WSCContextISF *wsc_ctx = wsc->wsc_ctx;

    isf_wsc_context_focus_out (wsc_ctx);

    if (wsc_ctx->keyboard) {
        wl_keyboard_destroy (wsc_ctx->keyboard);
        wsc_ctx->keyboard = NULL;
    }

    _fini_keysym2keycode (wsc_ctx);

    if (wsc_ctx->state) {
        xkb_state_unref (wsc_ctx->state);
        wsc_ctx->state = NULL;
    }

    if (wsc_ctx->keymap) {
        xkb_map_unref (wsc_ctx->keymap);
        wsc_ctx->keymap = NULL;
    }

    if (wsc_ctx->xkb_context) {
        xkb_context_unref (wsc_ctx->xkb_context);
        wsc_ctx->xkb_context = NULL;
    }

    if (wsc_ctx->im_ctx) {
        wl_input_method_context_destroy (wsc_ctx->im_ctx);
        wsc_ctx->im_ctx = NULL;
    }

    if (wsc_ctx->preedit_str) {
        free (wsc_ctx->preedit_str);
        wsc_ctx->preedit_str = NULL;
    }

    if (wsc_ctx->surrounding_text) {
        free (wsc_ctx->surrounding_text);
        wsc_ctx->surrounding_text = NULL;
    }

    if (wsc_ctx->language) {
        free (wsc_ctx->language);
        wsc_ctx->language = NULL;
    }

    isf_wsc_context_del (wsc_ctx);
    delete wsc_ctx;
    wsc->wsc_ctx = NULL;
}

static void
_wsc_im_show_input_panel(void *data, struct wl_input_method *input_method, struct wl_input_method_context *im_ctx)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc || !wsc->wsc_ctx) return;

    isf_wsc_context_input_panel_show (wsc->wsc_ctx);
}

static void
_wsc_im_hide_input_panel(void *data, struct wl_input_method *input_method, struct wl_input_method_context *im_ctx)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc || !wsc->wsc_ctx) return;

    isf_wsc_context_input_panel_hide (wsc->wsc_ctx);
}

static const struct wl_input_method_listener wsc_im_listener = {
    _wsc_im_activate,
    _wsc_im_deactivate,
    _wsc_im_show_input_panel,
    _wsc_im_hide_input_panel
};

static bool
_wsc_setup(struct weescim *wsc)
{
    Eina_Inlist *globals;
    struct wl_registry *registry;
    Ecore_Wl_Global *global;

    if (!wsc) return false;

    if (!(registry = ecore_wl_registry_get())) {
        LOGW ("failed to get wl_registry");
        return false;
    }

    if (!(globals = ecore_wl_globals_get())) {
        LOGW ("failed to get wl_globals");
        return false;
    }

    EINA_INLIST_FOREACH(globals, global) {
        if (strcmp (global->interface, "wl_input_method") == 0)
            wsc->im = (wl_input_method*)wl_registry_bind (registry, global->id, &wl_input_method_interface, 1);
    }

    if (wsc->im == NULL) {
        LOGW ("Failed because wl_input_method is null\n");
        return false;
    }

    /* Input method listener */
    LOGD ("Adding wl_input_method listener\n");

    if (wsc->im)
        wl_input_method_add_listener (wsc->im, &wsc_im_listener, wsc);
    else {
        LOGW ("Couldn't get wayland input method interface\n");
        return false;
    }

    return true;
}


//////////////////////////////wayland_panel_agent_module end//////////////////////////////////////////////////


WSCContextISF *
get_focused_ic ()
{
    return _focused_ic;
}

int
get_panel_client_id (void)
{
    return _panel_client_id;
}

void
get_language (char **language)
{
    *language = strdup (_language.c_str ());
}

static unsigned int
get_time (void)
{
    unsigned int tint;
    struct timeval tv;
    struct timezone tz;           /* is not used since ages */
    gettimeofday (&tv, &tz);
    tint = tv.tv_sec * 1000;
    tint = tint / 1000 * 1000;
    tint = tint + tv.tv_usec / 1000;
    return tint;
}

/* Function Implementations */
static WSCContextISFImpl *
new_ic_impl (WSCContextISF *parent)
{
    WSCContextISFImpl *impl = NULL;

    if (_free_ic_impl_list != NULL) {
        impl = _free_ic_impl_list;
        _free_ic_impl_list = _free_ic_impl_list->next;
    } else {
        impl = new WSCContextISFImpl;
        if (impl == NULL)
            return NULL;
    }

    impl->next = _used_ic_impl_list;
    _used_ic_impl_list = impl;

    impl->parent = parent;

    return impl;
}

static void
delete_ic_impl (WSCContextISFImpl *impl)
{
    WSCContextISFImpl *rec = _used_ic_impl_list, *last = 0;

    for (; rec != 0; last = rec, rec = rec->next) {
        if (rec == impl) {
            if (last != 0)
                last->next = rec->next;
            else
                _used_ic_impl_list = rec->next;

            rec->next = _free_ic_impl_list;
            _free_ic_impl_list = rec;

            if (rec->imdata) {
                free (rec->imdata);
                rec->imdata = NULL;
            }

            rec->imdata_size = 0;
            rec->parent = 0;
            rec->client_window = 0;
            rec->preedit_string = WideString ();
            rec->preedit_attrlist.clear ();

            return;
        }
    }
}

static void
delete_all_ic_impl (void)
{
    WSCContextISFImpl *it = _used_ic_impl_list;

    while (it != 0) {
        _used_ic_impl_list = it->next;
        delete it;
        it = _used_ic_impl_list;
    }

    it = _free_ic_impl_list;
    while (it != 0) {
        _free_ic_impl_list = it->next;
        delete it;
        it = _free_ic_impl_list;
    }
}

static WSCContextISF *
find_ic (int id)
{
    WSCContextISFImpl *rec = _used_ic_impl_list;

    while (rec != 0) {
        if (rec->parent && rec->parent->id == id)
            return rec->parent;
        rec = rec->next;
    }

    return 0;
}

static bool
check_valid_ic (WSCContextISF * ic)
{
    if (ic && ic->impl && ic->ctx)
        return true;
    else
        return false;
}

static Eina_Bool
check_symbol (Eina_Unicode ucode, Eina_Unicode symbols[], int symbol_num)
{
    for (int i = 0; i < symbol_num; i++) {
        // Check symbol
        if (ucode == symbols[i])
            return EINA_TRUE;
    }

    return EINA_FALSE;
}

static Eina_Bool
check_except_autocapital (Eina_Unicode *ustr, int cursor_pos)
{
    const char *except_str[] = {"e.g.", "E.g."};
    unsigned int i = 0, j = 0, len = 0;
    for (i = 0; i < (sizeof (except_str) / sizeof (except_str[0])); i++) {
        len = strlen (except_str[i]);
        if (cursor_pos < (int)len)
            continue;

        for (j = len; j > 0; j--) {
            if (ustr[cursor_pos-j] != except_str[i][len-j])
                break;
        }

        if (j == 0) return EINA_TRUE;
    }

    return EINA_FALSE;
}

static Eina_Bool
check_space_symbol (Eina_Unicode uchar)
{
    Eina_Unicode space_symbols[] = {' ', 0x00A0 /* no-break space */, 0x3000 /* ideographic space */};
    const int symbol_num = sizeof (space_symbols) / sizeof (space_symbols[0]);

    return check_symbol (uchar, space_symbols, symbol_num);
}

static void
autoperiod_insert (WSCContextISF *wsc_ctx)
{
    char *plain_str = NULL;
    int cursor_pos = 0;
    Eina_Unicode *ustr = NULL;
    char *fullstop_mark = NULL;

    if (autoperiod_allow == EINA_FALSE)
        return;

    if (!wsc_ctx) return;

    Ecore_IMF_Input_Panel_Layout layout = wsc_context_input_panel_layout_get (wsc_ctx);
    if (layout != ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL)
        return;

    if ((ecore_time_get () - space_key_time) > DOUBLE_SPACE_INTERVAL)
        goto done;

    wsc_context_surrounding_get (wsc_ctx, &plain_str, &cursor_pos);
    if (!plain_str) goto done;

    // Convert string from UTF-8 to unicode
    ustr = eina_unicode_utf8_to_unicode (plain_str, NULL);
    if (!ustr) goto done;

    if (cursor_pos < 2) goto done;

    if (check_space_symbol (ustr[cursor_pos-1]) &&
        !(iswpunct (ustr[cursor_pos-2]) || check_space_symbol (ustr[cursor_pos-2]))) {
        wsc_context_delete_surrounding (wsc_ctx, -1, 1);

        if (input_lang == INPUT_LANG_OTHER) {
            fullstop_mark = strdup (".");
        }
        else {
            wchar_t wbuf[2] = {0};
            wbuf[0] = __punctuations[input_lang].punc_code;

            WideString wstr = WideString (wbuf);
            fullstop_mark = strdup (utf8_wcstombs (wstr).c_str ());
        }

        wsc_context_commit_string (wsc_ctx, fullstop_mark);

        if (fullstop_mark) {
            free (fullstop_mark);
        }
    }

done:
    if (plain_str) free (plain_str);
    if (ustr) free (ustr);
    space_key_time = ecore_time_get ();
}

static Eina_Bool
analyze_surrounding_text (WSCContextISF *wsc_ctx)
{
    char *plain_str = NULL;
    Eina_Unicode puncs[] = {'\n','.', '!', '?', 0x00BF /* ¿ */, 0x00A1 /* ¡ */,
                            0x3002 /* 。 */, 0x06D4 /* Urdu */, 0x0964 /* Hindi */,
                            0x0589 /* Armenian */, 0x17D4 /* Khmer */, 0x104A /* Myanmar */};
    Eina_Unicode *ustr = NULL;
    Eina_Bool ret = EINA_FALSE;
    Eina_Bool detect_space = EINA_FALSE;
    int cursor_pos = 0;
    int i = 0;
    const int punc_num = sizeof (puncs) / sizeof (puncs[0]);
    WSCContextISF *context_scim;

    if (!wsc_ctx) return EINA_FALSE;
    context_scim = wsc_ctx;
    if (!context_scim || !context_scim->impl) return EINA_FALSE;

    switch (context_scim->impl->autocapital_type) {
        case ECORE_IMF_AUTOCAPITAL_TYPE_NONE:
            return EINA_FALSE;
        case ECORE_IMF_AUTOCAPITAL_TYPE_ALLCHARACTER:
            return EINA_TRUE;
        default:
            break;
    }

    if (context_scim->impl->cursor_pos == 0)
        return EINA_TRUE;

    if (context_scim->impl->preedit_updating)
        return EINA_FALSE;

    wsc_context_surrounding_get (wsc_ctx, &plain_str, &cursor_pos);
    if (!plain_str) goto done;

    if (cursor_pos == 0) {
        ret = EINA_TRUE;
        goto done;
    }

    // Convert string from UTF-8 to unicode
    ustr = eina_unicode_utf8_to_unicode (plain_str, NULL);
    if (!ustr) goto done;

    if (eina_unicode_strlen (ustr) < (size_t)cursor_pos) goto done;

    if (cursor_pos >= 1) {
        if (context_scim->impl->autocapital_type == ECORE_IMF_AUTOCAPITAL_TYPE_WORD) {
            // Check space or no-break space
            if (check_space_symbol (ustr[cursor_pos-1])) {
                ret = EINA_TRUE;
                goto done;
            }
        }

        // Check paragraph separator <PS> or carriage return  <br>
        if ((ustr[cursor_pos-1] == 0x2029) || (ustr[cursor_pos-1] == '\n')) {
            ret = EINA_TRUE;
            goto done;
        }

        for (i = cursor_pos; i > 0; i--) {
            // Check space or no-break space
            if (check_space_symbol (ustr[i-1])) {
                detect_space = EINA_TRUE;
                continue;
            }

            // Check punctuation and following the continuous space(s)
            if (detect_space && check_symbol (ustr[i-1], puncs, punc_num)) {
                if (check_except_autocapital (ustr, i))
                    ret = EINA_FALSE;
                else
                    ret = EINA_TRUE;

                goto done;
            } else {
                ret = EINA_FALSE;
                goto done;
            }
        }

        if ((i == 0) && (detect_space == EINA_TRUE)) {
            // continuous space(s) without any character
            ret = EINA_TRUE;
            goto done;
        }
    }

done:
    if (ustr) free (ustr);
    if (plain_str) free (plain_str);

    return ret;
}

Eina_Bool
caps_mode_check (WSCContextISF *wsc_ctx, Eina_Bool force, Eina_Bool noti)
{
    Eina_Bool uppercase;
    WSCContextISF *context_scim;

    if (get_keyboard_mode () == TOOLBAR_KEYBOARD_MODE) return EINA_FALSE;

    if (!wsc_ctx) return EINA_FALSE;
    context_scim = wsc_ctx;

    if (!context_scim || !context_scim->impl)
        return EINA_FALSE;

    if (context_scim->impl->next_shift_status == SHIFT_MODE_LOCK) return EINA_TRUE;

    Ecore_IMF_Input_Panel_Layout layout = wsc_context_input_panel_layout_get (wsc_ctx);
    if (layout != ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL)
        return EINA_FALSE;

    // Check autocapital type
    if (wsc_context_input_panel_caps_lock_mode_get (wsc_ctx)) {
        uppercase = EINA_TRUE;
    } else {
        if (autocap_allow == EINA_FALSE)
            return EINA_FALSE;

        if (analyze_surrounding_text (wsc_ctx)) {
            uppercase = EINA_TRUE;
        } else {
            uppercase = EINA_FALSE;
        }
    }

    if (force) {
        context_scim->impl->next_shift_status = uppercase ? SHIFT_MODE_ON : SHIFT_MODE_OFF;
        if (noti)
            isf_wsc_context_input_panel_caps_mode_set (wsc_ctx, uppercase);
    } else {
        if (context_scim->impl->next_shift_status != (uppercase ? SHIFT_MODE_ON : SHIFT_MODE_OFF)) {
            context_scim->impl->next_shift_status = uppercase ? SHIFT_MODE_ON : SHIFT_MODE_OFF;
            if (noti)
                isf_wsc_context_input_panel_caps_mode_set (wsc_ctx, uppercase);
        }
    }

    return uppercase;
}

static void
get_input_language ()
{
    char *input_lang_str = vconf_get_str (VCONFKEY_ISF_INPUT_LANGUAGE);
    if (!input_lang_str) return;

    input_lang = INPUT_LANG_OTHER;

    for (unsigned int i = 0; i < (sizeof (__punctuations) / sizeof (__punctuations[0])); i++) {
        if (strcmp (input_lang_str, __punctuations[i].code) == 0) {
            input_lang = __punctuations[i].lang;
            break;
        }
    }

    free (input_lang_str);
}

static void autoperiod_allow_changed_cb (keynode_t *key, void* data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    autoperiod_allow = vconf_keynode_get_bool (key);
}

static void autocapital_allow_changed_cb (keynode_t *key, void* data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    autocap_allow = vconf_keynode_get_bool (key);
}

static void input_language_changed_cb (keynode_t *key, void* data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    get_input_language ();
}

void context_scim_imdata_get (WSCContextISF *wsc_ctx, void* data, int* length)
{
    WSCContextISF* context_scim = wsc_ctx;
    LOGD ("");
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";

    if (context_scim && context_scim->impl) {
        if (data && context_scim->impl->imdata)
            memcpy (data, context_scim->impl->imdata, context_scim->impl->imdata_size);

        if (length)
            *length = context_scim->impl->imdata_size;
    }
}

static char *
insert_text (const char *text, uint32_t offset, const char *insert)
{
    int tlen = strlen (text), ilen = strlen (insert);
    char *new_text = (char*)malloc (tlen + ilen + 1);

    memcpy (new_text, text, offset);
    memcpy (new_text + offset, insert, ilen);
    memcpy (new_text + offset + ilen, text + offset, tlen - offset);
    new_text[tlen + ilen] = '\0';

    return new_text;
}

/* Public functions */
void
isf_wsc_context_init (void)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    int val;

    if (!_scim_initialized) {
        ecore_wl_init (NULL);
        initialize ();
        _scim_initialized = true;
        isf_wsc_input_panel_init ();
        //isf_wsc_context_set_hardware_keyboard_mode(context_scim);

        /* get autoperiod allow vconf value */
        if (vconf_get_bool (VCONFKEY_AUTOPERIOD_ALLOW_BOOL, &val) == 0) {
            if (val == EINA_TRUE)
                autoperiod_allow = EINA_TRUE;
        }

        vconf_notify_key_changed (VCONFKEY_AUTOPERIOD_ALLOW_BOOL, autoperiod_allow_changed_cb, NULL);

        /* get autocapital allow vconf value */
        if (vconf_get_bool (VCONFKEY_AUTOCAPITAL_ALLOW_BOOL, &val) == 0) {
            if (val == EINA_TRUE)
                autocap_allow = EINA_TRUE;
        }

        vconf_notify_key_changed (VCONFKEY_AUTOCAPITAL_ALLOW_BOOL, autocapital_allow_changed_cb, NULL);

        /* get input language vconf value */
        get_input_language ();

        vconf_notify_key_changed (VCONFKEY_ISF_INPUT_LANGUAGE, input_language_changed_cb, NULL);
    }
}

void
isf_wsc_context_shutdown (void)
{
    LOGD ("");
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";

    if (_scim_initialized) {
        _scim_initialized = false;

        vconf_ignore_key_changed (VCONFKEY_AUTOPERIOD_ALLOW_BOOL, autoperiod_allow_changed_cb);
        vconf_ignore_key_changed (VCONFKEY_AUTOCAPITAL_ALLOW_BOOL, autocapital_allow_changed_cb);
        vconf_ignore_key_changed (VCONFKEY_ISF_INPUT_LANGUAGE, input_language_changed_cb);

        isf_wsc_input_panel_shutdown ();
        finalize ();
        ecore_wl_shutdown ();
    }
}

void
isf_wsc_context_add (WSCContextISF *wsc_ctx)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* context_scim = wsc_ctx;

    if (!context_scim) return;
    context_scim->surrounding_text_fd_read_handler = NULL;
    context_scim->selection_text_fd_read_handler = NULL;
    context_scim->surrounding_text = NULL;
    context_scim->selection_text = NULL;

    context_scim->impl                      = new_ic_impl (context_scim);
    if (context_scim->impl == NULL) {
        std::cerr << "memory allocation failed in " << __FUNCTION__ << "\n";
        return;
    }

    context_scim->impl->client_window       = 0;
    context_scim->impl->preedit_caret       = 0;
    context_scim->impl->cursor_x            = 0;
    context_scim->impl->cursor_y            = 0;
    context_scim->impl->cursor_pos          = -1;
    context_scim->impl->cursor_top_y        = 0;
    context_scim->impl->is_on               = true;
    context_scim->impl->use_preedit         = _on_the_spot;
    context_scim->impl->preedit_started     = false;
    context_scim->impl->preedit_updating    = false;
    context_scim->impl->need_commit_preedit = false;

    if (!_ic_list)
        context_scim->next = NULL;
    else
        context_scim->next = _ic_list;
    _ic_list = context_scim;

    context_scim->impl->is_on = _config->read (String (SCIM_CONFIG_FRONTEND_IM_OPENED_BY_DEFAULT), context_scim->impl->is_on);

    g_info_manager->register_input_context (WAYLAND_MODULE_CLIENT_ID, context_scim->id, "");
    set_ic_capabilities (context_scim);
    SCIM_DEBUG_FRONTEND (2) << "input context created: id = " << context_scim->id << "\n";
}

void
isf_wsc_context_del (WSCContextISF *wsc_ctx)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");

    if (!_ic_list) return;

    WSCContextISF *context_scim = wsc_ctx;

    if (context_scim->selection_text_fd_read_handler) {
        int fd = ecore_main_fd_handler_fd_get(context_scim->selection_text_fd_read_handler);
        close(fd);
        ecore_main_fd_handler_del(context_scim->selection_text_fd_read_handler);
        context_scim->selection_text_fd_read_handler = NULL;
    }
    LOGD("");
    if (context_scim->selection_text) {
        free (context_scim->selection_text);
        context_scim->selection_text = NULL;
    }
    
    if (context_scim->surrounding_text_fd_read_handler) {
        int fd = ecore_main_fd_handler_fd_get(context_scim->surrounding_text_fd_read_handler);
        close(fd);
        ecore_main_fd_handler_del(context_scim->surrounding_text_fd_read_handler);
        context_scim->surrounding_text_fd_read_handler = NULL;
    }
    if (context_scim->surrounding_text) {
        free (context_scim->surrounding_text);
        context_scim->surrounding_text = NULL;
    }
    if (context_scim) {
        if (context_scim->id != _ic_list->id) {
            WSCContextISF * pre = _ic_list;
            WSCContextISF * cur = _ic_list->next;
            while (cur != NULL) {
                if (cur->id == context_scim->id) {
                    pre->next = cur->next;
                    break;
                }
                pre = cur;
                cur = cur->next;
            }
        } else {
            _ic_list = _ic_list->next;
        }
    }

    if (context_scim && context_scim->impl) {

        // Delete the instance.
        // FIXME:
        // In case the instance send out some helper event,
        // and this context has been focused out,
        // we need set the focused_ic to this context temporary.
        WSCContextISF* old_focused = _focused_ic;
        _focused_ic = context_scim;
        _focused_ic = old_focused;

        if (context_scim == _focused_ic) {
            g_info_manager->socket_turn_off ();
            g_info_manager->focus_out (WAYLAND_MODULE_CLIENT_ID, context_scim->id);
        }

        g_info_manager->remove_input_context (WAYLAND_MODULE_CLIENT_ID, context_scim->id);

        if (context_scim->impl) {
            delete_ic_impl (context_scim->impl);
            context_scim->impl = 0;
        }
    }

    if (context_scim == _focused_ic)
        _focused_ic = 0;

}

void
isf_wsc_context_focus_in (WSCContextISF *wsc_ctx)
{
    WSCContextISF* context_scim = wsc_ctx;
    LOGD ("");

    if (!context_scim)
        return;

    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__<< "(" << context_scim->id << ")...\n";

    if (_focused_ic) {
        if (_focused_ic == context_scim) {
            SCIM_DEBUG_FRONTEND(1) << "It's already focused.\n";
            return;
        }
        SCIM_DEBUG_FRONTEND(1) << "Focus out previous IC first: " << _focused_ic->id << "\n";
        isf_wsc_context_focus_out (_focused_ic);
    }

    if (_change_keyboard_mode_by_focus_move) {
        //if h/w keyboard mode, keyboard mode will be changed to s/w mode when the entry get the focus.
        LOGD ("Keyboard mode is changed H/W->S/W because of focus_in.\n");
        isf_wsc_context_set_keyboard_mode (wsc_ctx, TOOLBAR_HELPER_MODE);
    }

    if (context_scim && context_scim->impl) {
        _focused_ic = context_scim;

        context_scim->impl->is_on = _config->read (String (SCIM_CONFIG_FRONTEND_IM_OPENED_BY_DEFAULT), context_scim->impl->is_on);
        context_scim->impl->preedit_string.clear ();
        context_scim->impl->preedit_attrlist.clear ();
        context_scim->impl->preedit_caret = 0;
        context_scim->impl->preedit_started = false;

        g_info_manager->register_input_context (WAYLAND_MODULE_CLIENT_ID, context_scim->id, "");

        set_ic_capabilities (context_scim);

        //FIXME: modify the parameter ic->impl->si->get_factory_uuid ()
        g_info_manager->focus_in (WAYLAND_MODULE_CLIENT_ID, context_scim->id, "");
        if (context_scim->impl->is_on) {
            g_info_manager->socket_turn_on ();
            //            _panel_client.hide_preedit_string (context_scim->id);
            //            _panel_client.hide_aux_string (context_scim->id);
            //            _panel_client.hide_lookup_table (context_scim->id);
            //FIXME
            #if 0 //REMOVE_SCIM_LAUNCHER
            context_scim->impl->si->set_layout (wsc_context_input_panel_layout_get (wsc_ctx));
            context_scim->impl->si->set_prediction_allow (context_scim->impl->prediction_allow);

            if (context_scim->impl->imdata)
                context_scim->impl->si->set_imdata ((const char*)context_scim->impl->imdata, context_scim->impl->imdata_size);
            #endif
        } else {
            g_info_manager->socket_turn_off ();
        }

        g_info_manager->get_active_helper_option (WAYLAND_MODULE_CLIENT_ID, _active_helper_option);

        /* At the moment we received focus_in, our surrounding text has not been updated yet -
        which means it will always turn Shift key on, resulting the whole keyboard blinking.
        This is only a temporary solution - the caps_mode_check() needs be executed on
        client application side. The code below, will still be needed when we implement the
        auto capitalization logic on immodule, so just commenting out */
        /*
        if (caps_mode_check (wsc_ctx, EINA_FALSE, EINA_TRUE) == EINA_FALSE) {
            context_scim->impl->next_shift_status = 0;
        }

        */
    }

    LOGD ("ctx : %p\n", wsc_ctx);
}

void
isf_wsc_context_focus_out (WSCContextISF *wsc_ctx)
{
    WSCContextISF* context_scim = wsc_ctx;
    LOGD ("");

    if (!context_scim) return;

    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "(" << context_scim->id << ")...\n";

    if (context_scim && context_scim->impl && context_scim == _focused_ic) {

        LOGD ("ctx : %p\n", wsc_ctx);

        if (context_scim->impl->need_commit_preedit) {
            _hide_preedit_string (context_scim->id, false);

            wsc_context_commit_preedit_string (context_scim);
            g_info_manager->socket_reset_input_context (WAYLAND_MODULE_CLIENT_ID, context_scim->id);
        }

        context_scim->impl->cursor_pos = -1;
//          if (context_scim->impl->shared_si) context_scim->impl->si->reset ();
        g_info_manager->focus_out (WAYLAND_MODULE_CLIENT_ID, context_scim->id);

        _focused_ic = 0;
    }
    _x_key_event_is_valid = false;
}

void
isf_wsc_context_preedit_string_get (WSCContextISF *wsc_ctx, char** str, int *cursor_pos)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* context_scim = wsc_ctx;

    if (context_scim && context_scim->impl && context_scim->impl->is_on) {
        String mbs = utf8_wcstombs (context_scim->impl->preedit_string);

        if (str) {
            if (mbs.length ())
                *str = strdup (mbs.c_str ());
            else
                *str = strdup ("");
        }

        if (cursor_pos) {
            //*cursor_pos = context_scim->impl->preedit_caret;
            mbs = utf8_wcstombs (
                context_scim->impl->preedit_string.substr (0, context_scim->impl->preedit_caret));
            *cursor_pos = mbs.length ();
        }
    } else {
        if (str)
            *str = strdup ("");

        if (cursor_pos)
            *cursor_pos = 0;
    }
}

void
isf_wsc_context_autocapital_type_set (WSCContextISF* wsc_ctx, Ecore_IMF_Autocapital_Type autocapital_type)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " = " << autocapital_type << "...\n";
    LOGD ("");
    WSCContextISF* context_scim = wsc_ctx;

    if (context_scim && context_scim->impl && context_scim->impl->autocapital_type != autocapital_type) {
        context_scim->impl->autocapital_type = autocapital_type;

        if (context_scim == _focused_ic) {
            LOGD ("ctx : %p. set autocapital type : %d\n", wsc_ctx, autocapital_type);
            //FIXME:add this interface
            //_info_manager->set_autocapital_type (autocapital_type);
        }
    }
}

void
isf_wsc_context_bidi_direction_set (WSCContextISF* wsc_ctx, Ecore_IMF_BiDi_Direction direction)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *context_scim = wsc_ctx;

    if (context_scim && context_scim->impl) {
        if (context_scim->impl->bidi_direction != direction) {
            context_scim->impl->bidi_direction = direction;

            if (context_scim == _focused_ic) {
                LOGD ("ctx : %p, bidi direction : %#x\n", wsc_ctx, direction);
                panel_req_update_bidi_direction (context_scim, direction);
            }
        }
    }
}

#ifdef _TV
static
bool is_number_key(const char *str)
{
    if (!str) return false;

    int result = atoi(str);

    if (result == 0)
    {
        if (!strcmp(str, "0"))
            return true;
        else
            return false;
    }
    else
        return true;
}
#endif

void
isf_wsc_context_filter_key_event (WSCContextISF* wsc_ctx,
                                  uint32_t serial,
                                  uint32_t timestamp, uint32_t keycode, uint32_t symcode,
                                  char *keyname,
                                  enum wl_keyboard_key_state state)

{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");

    if (!wsc_ctx) return;

    KeyEvent key(symcode, wsc_ctx->modifiers);
    bool ignore_key = filter_keys (keyname, SCIM_CONFIG_HOTKEYS_FRONTEND_IGNORE_KEY);

    if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
        key.mask = SCIM_KEY_ReleaseMask;
    } else if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        if (!ignore_key) {
            /* Hardware input detect code */
#ifdef _TV
            if (get_keyboard_mode() == TOOLBAR_HELPER_MODE &&
                timestamp > 1 &&
                _support_hw_keyboard_mode &&
                strcmp(keyname, "Down") &&
                strcmp(keyname, "KP_Down") &&
                strcmp(keyname, "Up") &&
                strcmp(keyname, "KP_Up") &&
                strcmp(keyname, "Right") &&
                strcmp(keyname, "KP_Right") &&
                strcmp(keyname, "Left") &&
                strcmp(keyname, "KP_Left") &&
                strcmp(keyname, "Return") &&
                strcmp(keyname, "Pause") &&
                strcmp(keyname, "NoSymbol") &&
                strncmp(keyname, "XF86", 4) &&
                !is_number_key(keyname)) {
#else
            if (get_keyboard_mode() == TOOLBAR_HELPER_MODE && timestamp > 1
                && _support_hw_keyboard_mode && key.code != 0x1008ff26
                && key.code != 0xFF69) {
                /* XF86back, Cancel (Power + Volume down) key */
#endif
                isf_wsc_context_set_keyboard_mode (wsc_ctx, TOOLBAR_KEYBOARD_MODE);
                ISF_SAVE_LOG ("Changed keyboard mode from S/W to H/W (code: %x, name: %s)\n", key.code, keyname);
                LOGD ("Hardware keyboard mode, active helper option: %d\n", _active_helper_option);
            }
        }
    }

    if (!ignore_key) {

        if (!_focused_ic || !_focused_ic->impl || !_focused_ic->impl->is_on) {
            LOGD ("ic is off");
        } else {
            static uint32 _serial = 0;
            g_info_manager->process_key_event (key, ++_serial);
        }
    } else {
        send_wl_key_event (wsc_ctx, key, false);
    }
}

static void
wsc_commit_preedit (WSCContextISF* wsc_ctx)
{
    LOGD ("");
    char* surrounding_text;

    if (!wsc_ctx || !wsc_ctx->preedit_str ||
        strlen (wsc_ctx->preedit_str) == 0)
        return;

    wl_input_method_context_cursor_position (wsc_ctx->im_ctx,
                                             0, 0);

    wl_input_method_context_commit_string (wsc_ctx->im_ctx,
                                           wsc_ctx->serial,
                                           wsc_ctx->preedit_str);

    if (wsc_ctx->surrounding_text) {
        surrounding_text = insert_text (wsc_ctx->surrounding_text,
                                        wsc_ctx->surrounding_cursor,
                                        wsc_ctx->preedit_str);

        free (wsc_ctx->surrounding_text);
        wsc_ctx->surrounding_text = surrounding_text;
        wsc_ctx->surrounding_cursor += strlen (wsc_ctx->preedit_str);
    } else {
        wsc_ctx->surrounding_text = strdup (wsc_ctx->preedit_str);
        wsc_ctx->surrounding_cursor = strlen (wsc_ctx->preedit_str);
    }

    if (wsc_ctx->preedit_str)
        free (wsc_ctx->preedit_str);

    wsc_ctx->preedit_str = strdup ("");
}

static void
wsc_send_preedit (WSCContextISF* wsc_ctx, int32_t cursor)
{
    LOGD ("");

    if (!wsc_ctx) return;

    uint32_t index = strlen (wsc_ctx->preedit_str);

    if (wsc_ctx && wsc_ctx->impl && wsc_ctx->impl->is_on) {
        String mbs = utf8_wcstombs (wsc_ctx->impl->preedit_string);

        if (!wsc_ctx->impl->preedit_attrlist.empty()) {
            if (mbs.length ()) {
                uint32_t preedit_style = WL_TEXT_INPUT_PREEDIT_STYLE_DEFAULT;
                int start_index, end_index;
                int wlen = wsc_ctx->impl->preedit_string.length ();
                AttributeList::const_iterator i;
                bool *attrs_flag = new bool [mbs.length ()];
                memset (attrs_flag, 0, mbs.length () * sizeof (bool));
                for (i = wsc_ctx->impl->preedit_attrlist.begin ();
                    i != wsc_ctx->impl->preedit_attrlist.end (); ++i) {
                    start_index = i->get_start ();
                    end_index = i->get_end ();
                    if (end_index <= wlen && start_index < end_index && i->get_type () != SCIM_ATTR_DECORATE_NONE) {
                        start_index = g_utf8_offset_to_pointer (mbs.c_str (), i->get_start ()) - mbs.c_str ();
                        end_index = g_utf8_offset_to_pointer (mbs.c_str (), i->get_end ()) - mbs.c_str ();
                        if (i->get_type () == SCIM_ATTR_DECORATE) {
                            switch (i->get_value ())
                            {
                                case SCIM_ATTR_DECORATE_UNDERLINE:
                                    preedit_style = WL_TEXT_INPUT_PREEDIT_STYLE_UNDERLINE;
                                    break;
                                case SCIM_ATTR_DECORATE_REVERSE:
                                    preedit_style = WL_TEXT_INPUT_PREEDIT_STYLE_SELECTION;
                                    break;
                                case SCIM_ATTR_DECORATE_HIGHLIGHT:
                                    preedit_style = WL_TEXT_INPUT_PREEDIT_STYLE_HIGHLIGHT;
                                    break;
                                case SCIM_ATTR_DECORATE_BGCOLOR1:
                                case SCIM_ATTR_DECORATE_BGCOLOR2:
                                case SCIM_ATTR_DECORATE_BGCOLOR3:
                                case SCIM_ATTR_DECORATE_BGCOLOR4:
                                default:
                                    preedit_style = WL_TEXT_INPUT_PREEDIT_STYLE_DEFAULT;
                                    break;
                            }

                            if (preedit_style)
                                wl_input_method_context_preedit_styling (wsc_ctx->im_ctx,
                                                                         start_index,
                                                                         end_index,
                                                                         preedit_style);
                            switch (i->get_value ())
                            {
                                case SCIM_ATTR_DECORATE_UNDERLINE:
                                case SCIM_ATTR_DECORATE_REVERSE:
                                case SCIM_ATTR_DECORATE_HIGHLIGHT:
                                case SCIM_ATTR_DECORATE_BGCOLOR1:
                                case SCIM_ATTR_DECORATE_BGCOLOR2:
                                case SCIM_ATTR_DECORATE_BGCOLOR3:
                                case SCIM_ATTR_DECORATE_BGCOLOR4:
                                    // Record which character has attribute.
                                    for (int pos = start_index; pos < end_index; ++pos)
                                        attrs_flag [pos] = 1;
                                    break;
                                default:
                                    break;
                            }
                        } else if (i->get_type () == SCIM_ATTR_FOREGROUND) {
                            SCIM_DEBUG_FRONTEND(4) << "SCIM_ATTR_FOREGROUND\n";
                        } else if (i->get_type () == SCIM_ATTR_BACKGROUND) {
                            SCIM_DEBUG_FRONTEND(4) << "SCIM_ATTR_BACKGROUND\n";
                        }
                    }
                }
                // Add underline for all characters which don't have attribute.
                for (unsigned int pos = 0; pos < mbs.length (); ++pos) {
                    if (!attrs_flag [pos]) {
                        int begin_pos = pos;
                        while (pos < mbs.length () && !attrs_flag [pos])
                            ++pos;
                        // use REVERSE style as default
                        preedit_style = WL_TEXT_INPUT_PREEDIT_STYLE_UNDERLINE;
                        start_index = begin_pos;
                        end_index = pos;

                        wl_input_method_context_preedit_styling (wsc_ctx->im_ctx,
                                                                 start_index,
                                                                 end_index,
                                                                 preedit_style);
                    }
                }
                delete [] attrs_flag;
            }
        }
    } else {
        if (!wsc_ctx->impl->preedit_attrlist.empty())
            wsc_ctx->impl->preedit_attrlist.clear();
    }

    if (cursor > 0)
        index = cursor;

    wl_input_method_context_preedit_cursor (wsc_ctx->im_ctx, index);
    wl_input_method_context_preedit_string (wsc_ctx->im_ctx,
                                            wsc_ctx->serial,
                                            wsc_ctx->preedit_str,
                                            wsc_ctx->preedit_str);
}

bool wsc_context_surrounding_get (WSCContextISF *wsc_ctx, char **text, int *cursor_pos)
{
    if (!wsc_ctx)
        return false;

    if (text) {
        if (wsc_ctx->surrounding_text)
            *text = strdup (wsc_ctx->surrounding_text);
        else
            *text = strdup ("");
    }

    if (cursor_pos)
        *cursor_pos = wsc_ctx->surrounding_cursor;

    return true;
}

Ecore_IMF_Input_Panel_Layout wsc_context_input_panel_layout_get (WSCContextISF *wsc_ctx)
{
    Ecore_IMF_Input_Panel_Layout layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;

    if (!wsc_ctx)
        return layout;

    switch (wsc_ctx->content_purpose) {
        case WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS:
        case WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS_SIGNED:
        case WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS_DECIMAL:
        case WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS_SIGNEDDECIMAL:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_NUMBER:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBER;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_DATE:
        case WL_TEXT_INPUT_CONTENT_PURPOSE_TIME:
        case WL_TEXT_INPUT_CONTENT_PURPOSE_DATETIME:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_DATETIME;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_PHONE:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_PHONENUMBER;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_URL:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_URL;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_EMAIL:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_EMAIL;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_PASSWORD:
        case WL_TEXT_INPUT_CONTENT_PURPOSE_PASSWORD_DIGITS:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_HEX:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_HEX;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_TERMINAL:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_TERMINAL;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_IP:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_IP;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_EMOTICON:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_EMOTICON;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_NORMAL:
        case WL_TEXT_INPUT_CONTENT_PURPOSE_FILENAME:
        case WL_TEXT_INPUT_CONTENT_PURPOSE_NAME:
        default:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;
            break;
    }

    return layout;
}

int wsc_context_input_panel_layout_variation_get (WSCContextISF *wsc_ctx)
{
    int layout_variation = 0;

    if (!wsc_ctx)
        return layout_variation;

    switch (wsc_ctx->content_purpose) {
        case WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS:
            layout_variation = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_NORMAL;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS_SIGNED:
            layout_variation = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_SIGNED;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS_DECIMAL:
            layout_variation = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_DECIMAL;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS_SIGNEDDECIMAL:
            layout_variation = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_SIGNED_AND_DECIMAL;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_PASSWORD:
            layout_variation = ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD_VARIATION_NORMAL;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_PASSWORD_DIGITS:
            layout_variation = ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD_VARIATION_NUMBERONLY;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_NORMAL:
            layout_variation = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL_VARIATION_NORMAL;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_FILENAME:
            layout_variation = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL_VARIATION_FILENAME;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_NAME:
            layout_variation = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL_VARIATION_PERSON_NAME;
            break;
        default:
            layout_variation = 0;
            break;
    }

    return layout_variation;
}

Ecore_IMF_Autocapital_Type wsc_context_autocapital_type_get (WSCContextISF *wsc_ctx)
{
    Ecore_IMF_Autocapital_Type autocapital_type = ECORE_IMF_AUTOCAPITAL_TYPE_NONE;

    if (!wsc_ctx)
        return autocapital_type;

    if (wsc_ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_AUTO_CAPITALIZATION)
        autocapital_type = ECORE_IMF_AUTOCAPITAL_TYPE_SENTENCE;
    else if (wsc_ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_WORD_CAPITALIZATION)
        autocapital_type = ECORE_IMF_AUTOCAPITAL_TYPE_WORD;
    else if (wsc_ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_UPPERCASE)
        autocapital_type = ECORE_IMF_AUTOCAPITAL_TYPE_ALLCHARACTER;
    else
        autocapital_type = ECORE_IMF_AUTOCAPITAL_TYPE_NONE;

    return autocapital_type;
}

bool wsc_context_input_panel_caps_lock_mode_get (WSCContextISF *wsc_ctx)
{
    if (!wsc_ctx)
        return false;

    if (wsc_ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_UPPERCASE)
        return true;

    return false;
}

Ecore_IMF_Input_Panel_Lang wsc_context_input_panel_language_get (WSCContextISF *wsc_ctx)
{
    Ecore_IMF_Input_Panel_Lang language = ECORE_IMF_INPUT_PANEL_LANG_AUTOMATIC;

    if (!wsc_ctx)
        return language;

    if (wsc_ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_LATIN)
        language = ECORE_IMF_INPUT_PANEL_LANG_ALPHABET;
    else
        language = ECORE_IMF_INPUT_PANEL_LANG_AUTOMATIC;

    return language;
}

bool wsc_context_input_panel_password_mode_get (WSCContextISF *wsc_ctx)
{
    if (wsc_ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_PASSWORD)
        return true;

    if (wsc_context_input_panel_layout_get (wsc_ctx) == ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD)
        return true;

    return false;
}

Ecore_IMF_Input_Hints wsc_context_input_hint_get (WSCContextISF *wsc_ctx)
{
    int input_hint = ECORE_IMF_INPUT_HINT_NONE;

    if (!wsc_ctx)
        return (Ecore_IMF_Input_Hints)input_hint;

    if (wsc_ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_SENSITIVE_DATA)
        input_hint |= ECORE_IMF_INPUT_HINT_SENSITIVE_DATA;
    else
        input_hint &= ~ECORE_IMF_INPUT_HINT_SENSITIVE_DATA;

    if (wsc_ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_AUTO_COMPLETION)
        input_hint |= ECORE_IMF_INPUT_HINT_AUTO_COMPLETE;
    else
        input_hint &= ~ECORE_IMF_INPUT_HINT_AUTO_COMPLETE;

    return (Ecore_IMF_Input_Hints)input_hint;
}

Eina_Bool wsc_context_prediction_allow_get (WSCContextISF *wsc_ctx)
{
    LOGD ("");

    if (!wsc_ctx)
        return EINA_FALSE;

    if (wsc_ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_AUTO_COMPLETION)
        return EINA_TRUE;
    else
        return EINA_FALSE;
}

void wsc_context_delete_surrounding (WSCContextISF *wsc_ctx, int offset, int len)
{
    LOGD ("offset = %d, len = %d", offset, len);

    if (!wsc_ctx)
        return;

    wl_input_method_context_delete_surrounding_text (wsc_ctx->im_ctx, offset, len);
}

void wsc_context_set_selection (WSCContextISF *wsc_ctx, int start, int end)
{
    LOGD ("");

    if (!wsc_ctx)
        return;

    wl_input_method_context_selection_region (wsc_ctx->im_ctx, wsc_ctx->serial, start, end);
}

void wsc_context_commit_string (WSCContextISF *wsc_ctx, const char *str)
{
    LOGD ("");

    if (!wsc_ctx)
        return;

    if (wsc_ctx->preedit_str) {
        free (wsc_ctx->preedit_str);
        wsc_ctx->preedit_str = NULL;
    }

    wsc_ctx->preedit_str = strdup (str);
    wsc_commit_preedit (wsc_ctx);
}

void wsc_context_commit_preedit_string (WSCContextISF *wsc_ctx)
{
    LOGD ("");
    char* preedit_str = NULL;
    int cursor_pos = 0;

    if (!wsc_ctx)
        return;

    isf_wsc_context_preedit_string_get (wsc_ctx, &preedit_str, &cursor_pos);

    if (wsc_ctx->preedit_str) {
        free (wsc_ctx->preedit_str);
        wsc_ctx->preedit_str = NULL;
    }

    wsc_ctx->preedit_str = preedit_str;
    wsc_commit_preedit (wsc_ctx);
}

void wsc_context_send_preedit_string (WSCContextISF *wsc_ctx)
{
    LOGD ("");
    char* preedit_str = NULL;
    int cursor_pos = 0;

    if (!wsc_ctx)
        return;

    isf_wsc_context_preedit_string_get (wsc_ctx, &preedit_str, &cursor_pos);

    if (wsc_ctx->preedit_str) {
        free (wsc_ctx->preedit_str);
        wsc_ctx->preedit_str = NULL;
    }

    wsc_ctx->preedit_str = preedit_str;
    wsc_send_preedit (wsc_ctx, cursor_pos);
}

void wsc_context_send_key (WSCContextISF *wsc_ctx, uint32_t keysym, uint32_t modifiers, uint32_t time, bool press)
{
    LOGD ("");

    if (!wsc_ctx || !wsc_ctx->im_ctx)
        return;

    wl_input_method_context_keysym (wsc_ctx->im_ctx, wsc_ctx->serial, time,
            keysym, press ? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED, modifiers);
}

static void
set_ic_capabilities (WSCContextISF *ic)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
#if 0 //FIXME
    if (ic && ic->impl) {
        unsigned int cap = SCIM_CLIENT_CAP_ALL_CAPABILITIES;

        if (!_on_the_spot || !ic->impl->use_preedit)
            cap -= SCIM_CLIENT_CAP_ONTHESPOT_PREEDIT;

        //FIXME:add this interface
        //_info_manager->update_client_capabilities (cap);
    }
#endif
}

//useless
#if 0
/* Panel Requestion functions. */
static void
panel_req_show_help (WSCContextISF *ic)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    String help;

    help =  String (_("Smart Common Input Method platform ")) +
            String (SCIM_VERSION) +
            String (_("\n(C) 2002-2005 James Su <suzhe@tsinghua.org.cn>\n\n"));

    if (ic && ic->impl && ic->impl->si) {
        IMEngineFactoryPointer sf = _backend->get_factory (ic->impl->si->get_factory_uuid ());
        if (sf) {
            help += utf8_wcstombs (sf->get_name ());
            help += String (_(":\n\n"));

            help += utf8_wcstombs (sf->get_help ());
            help += String (_("\n\n"));

            help += utf8_wcstombs (sf->get_credits ());
        }

        g_info_manager->socket_show_help (help);
    }
}
#endif

static void
panel_req_update_bidi_direction   (WSCContextISF *ic, int direction)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic)
        g_info_manager->update_ise_bidi_direction (WAYLAND_MODULE_CLIENT_ID, direction);
}


static bool
filter_keys (const char *keyname, const char *config_path)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");

    if (!keyname)
        return false;

    std::vector <String> keys;
    scim_split_string_list (keys, _config->read (String (config_path), String ("")), ',');

    for (unsigned int i = 0; i < keys.size (); ++i) {
        if (!strcmp (keyname, keys [i].c_str ())) {
            return true;
        }
    }

    return false;
}

static void
panel_initialize (void)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    String display_name;
    {
        const char *p = getenv ("DISPLAY");
        if (p) display_name = String (p);
    }
    g_info_manager->add_client (WAYLAND_MODULE_CLIENT_ID, 2, FRONTEND_CLIENT);
    _panel_client_id = WAYLAND_MODULE_CLIENT_ID;
    g_info_manager->register_panel_client (_panel_client_id, _panel_client_id);
    WSCContextISF* context_scim = _ic_list;

    while (context_scim != NULL) {
        //FIXME:modify the parameter
        g_info_manager->register_input_context (WAYLAND_MODULE_CLIENT_ID, context_scim->id, "");

        context_scim = context_scim->next;
    }

    if (_focused_ic) {
        _focused_ic = 0;
    }
}

static void
panel_finalize (void)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    g_info_manager->del_client (WAYLAND_MODULE_CLIENT_ID);
}

static void
panel_slot_update_preedit_caret (int context, int caret)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " caret=" << caret << " ic=" << ic << "\n";

    if (ic && ic->impl && _focused_ic == ic && ic->impl->preedit_caret != caret) {
        ic->impl->preedit_caret = caret;
        if (ic->impl->use_preedit) {
            if (!ic->impl->preedit_started) {
                if (check_valid_ic (ic))
                    ic->impl->preedit_started = true;
            }
        } else {
            g_info_manager->socket_update_preedit_caret (caret);
        }
    }
}

static Eina_Bool
feed_key_event (WSCContextISF *ic, const KeyEvent &key, bool fake)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");

    if (key.code <= 0x7F ||
        (key.code >= SCIM_KEY_BackSpace && key.code <= SCIM_KEY_Delete) ||
        (key.code >= SCIM_KEY_Home && key.code <= SCIM_KEY_Hyper_R)) {
        // ascii code and function keys
        send_wl_key_event (ic, key, fake);
        return EINA_TRUE;
    } else {
        return EINA_FALSE;
    }
}

static void
panel_slot_process_key_event (int context, const KeyEvent &key)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " key=" << key.get_key_string () << " ic=" << ic << "\n";

    if (!(ic && ic->impl))
        return;

    if ((_focused_ic != NULL) && (_focused_ic != ic))
        return;

    //FIXME: filter_hotkeys should be added
    //if (filter_hotkeys (ic, _key) == false && process_key) {
        //if (!_focused_ic || !_focused_ic->impl->is_on) {
            char code = key.get_ascii_code ();
            if (key.is_key_press ()) {
                if (isgraph (code)) {
                    char string[2] = {0};
                    snprintf (string, sizeof (string), "%c", code);

                    if (strlen (string) != 0) {
                        wsc_context_commit_string (ic, string);
                        caps_mode_check (ic, EINA_FALSE, EINA_TRUE);
                    }
                } else {
                    if (key.code == SCIM_KEY_space ||
                        key.code == SCIM_KEY_KP_Space)
                        autoperiod_insert (ic);
                }
            }

            if (!isgraph (code)) {
                feed_key_event (ic, key, false);
            }
        //}
    //}
}

static void
panel_slot_commit_string (int context, const WideString &wstr)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " str=" << utf8_wcstombs (wstr) << " ic=" << ic << "\n";

    if (ic && ic->impl) {
        if (_focused_ic != ic)
            return;

        if (utf8_wcstombs (wstr) == String (" ") || utf8_wcstombs (wstr) == String ("　"))
            autoperiod_insert (ic);

        if (ic->impl->need_commit_preedit)
            _hide_preedit_string (ic->id, false);
        wsc_context_commit_string (ic, utf8_wcstombs (wstr).c_str ());
    }
}

static void
panel_slot_forward_key_event (int context, const KeyEvent &key)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " key=" << key.get_key_string () << " ic=" << ic << "\n";

    if (!(ic && ic->impl))
        return;

    if ((_focused_ic != NULL) && (_focused_ic != ic))
        return;

    if (strlen (key.get_key_string ().c_str ()) >= 116)
        return;

    feed_key_event (ic, key, true);
}

static void
_show_preedit_string (int context)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << "\n";

    if (ic && ic->impl && _focused_ic == ic) {
        if (!ic->impl->is_on)
            ic->impl->is_on = true;

        if (ic->impl->use_preedit) {
            if (!ic->impl->preedit_started) {
                if (check_valid_ic (ic)) {
                    ic->impl->preedit_started     = true;
                    ic->impl->need_commit_preedit = true;
                }
            }
        } else {
            g_info_manager->socket_show_preedit_string ();
        }
    }
}

static void
_hide_preedit_string (int context, bool update_preedit)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = find_ic (context);

    if (ic && ic->impl && _focused_ic == ic) {
        if (!ic->impl->is_on)
            ic->impl->is_on = true;

        bool emit = false;
        if (ic->impl->preedit_string.length ()) {
            ic->impl->preedit_string = WideString ();
            ic->impl->preedit_caret  = 0;
            ic->impl->preedit_attrlist.clear ();
            emit = true;
        }
        if (ic->impl->use_preedit) {
            if (update_preedit && emit) {
                if (!check_valid_ic (ic))
                    return;
            }
            if (ic->impl->preedit_started) {
                if (check_valid_ic (ic)) {
                    ic->impl->preedit_started     = false;
                    ic->impl->need_commit_preedit = false;
                }
            }
            wsc_context_send_preedit_string (ic);
        } else {
            g_info_manager->socket_hide_preedit_string ();
        }
    }
}

static void
_update_preedit_string (int context,
                                  const WideString    &str,
                                  const AttributeList &attrs,
                                  int caret)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = find_ic (context);

    if (ic && ic->impl && _focused_ic == ic) {
        if (!ic->impl->is_on)
            ic->impl->is_on = true;

        if (ic->impl->preedit_string != str || str.length ()) {
            ic->impl->preedit_string   = str;
            ic->impl->preedit_attrlist = attrs;

            if (ic->impl->use_preedit) {
                if (!ic->impl->preedit_started) {
                    if (!check_valid_ic (ic))
                        return;

                    ic->impl->preedit_started = true;
                    ic->impl->need_commit_preedit = true;
                }
                if (caret >= 0 && caret <= (int)str.length ())
                    ic->impl->preedit_caret    = caret;
                else
                    ic->impl->preedit_caret    = str.length ();
                ic->impl->preedit_updating = true;
                if (check_valid_ic (ic))
                    ic->impl->preedit_updating = false;
                wsc_context_send_preedit_string (ic);
            } else {
                String _str = utf8_wcstombs (str);
                g_info_manager->socket_update_preedit_string (_str, attrs, (uint32)caret);
            }
        }
    }
}

void
initialize (void)
{

    LOGD ("Initializing Wayland ISF IMModule...\n");

    // Get system language.
    _language = scim_get_locale_language (scim_get_current_locale ());

    panel_initialize ();
}

static void
finalize (void)
{
    LOGD ("Finalizing Ecore ISF IMModule...\n");

    SCIM_DEBUG_FRONTEND(2) << "Finalize all IC partially.\n";
    while (_used_ic_impl_list) {
        // In case in "shared input method" mode,
        // all contexts share only one instance,
        // so we need point the reference pointer correctly before finalizing.
        #if 0 //REMOVE
        if (_used_ic_impl_list->si) {
            _used_ic_impl_list->si->set_frontend_data (static_cast <void*> (_used_ic_impl_list->parent));
        }
        #endif
        if (_used_ic_impl_list->parent && _used_ic_impl_list->parent->ctx) {
            isf_wsc_context_del (_used_ic_impl_list->parent);
        }
    }

    delete_all_ic_impl ();

    SCIM_DEBUG_FRONTEND(2) << " Releasing Config...\n";
    _config.reset ();
    _focused_ic = NULL;
    _ic_list = NULL;

    _scim_initialized = false;
    panel_finalize ();
}

#define MOD_SHIFT_MASK		0x01
#define MOD_ALT_MASK		0x02
#define MOD_CONTROL_MASK	0x04

static uint32_t _keyname_to_keysym (uint32_t keyname, uint32_t *modifiers)
{
    if (!modifiers)
        return keyname;

    if ((keyname >= '0' && keyname <= '9') ||
        (keyname >= 'a' && keyname <= 'z')) {
        return keyname;
    } else if (keyname >= 'A' && keyname <= 'Z') {
        *modifiers |= MOD_SHIFT_MASK;
        return keyname + 32;
    }

    switch (keyname) {
        case '!':
            *modifiers |= MOD_SHIFT_MASK;
            return '1';
        case '@':
            *modifiers |= MOD_SHIFT_MASK;
            return '2';
        case '#':
            *modifiers |= MOD_SHIFT_MASK;
            return '3';
        case '$':
            *modifiers |= MOD_SHIFT_MASK;
            return '4';
        case '%':
            *modifiers |= MOD_SHIFT_MASK;
            return '5';
        case '^':
            *modifiers |= MOD_SHIFT_MASK;
            return '6';
        case '&':
            *modifiers |= MOD_SHIFT_MASK;
            return '7';
        case '*':
            *modifiers |= MOD_SHIFT_MASK;
            return '8';
        case '(':
            *modifiers |= MOD_SHIFT_MASK;
            return '9';
        case ')':
            *modifiers |= MOD_SHIFT_MASK;
            return '0';
        case '_':
            *modifiers |= MOD_SHIFT_MASK;
            return '-';
        case '+':
            *modifiers |= MOD_SHIFT_MASK;
            return '=';
        case '{':
            *modifiers |= MOD_SHIFT_MASK;
            return '[';
        case '}':
            *modifiers |= MOD_SHIFT_MASK;
            return ']';
        case '|':
            *modifiers |= MOD_SHIFT_MASK;
            return '\\';
        case ':':
            *modifiers |= MOD_SHIFT_MASK;
            return ';';
        case '\"':
            *modifiers |= MOD_SHIFT_MASK;
            return '\'';
        case '<':
            *modifiers |= MOD_SHIFT_MASK;
            return ',';
        case '>':
            *modifiers |= MOD_SHIFT_MASK;
            return '.';
        case '?':
            *modifiers |= MOD_SHIFT_MASK;
            return '/';
        default:
            return keyname;
    }
}

static void send_wl_key_event (WSCContextISF *ic, const KeyEvent &key, bool fake)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    uint32_t time = 0;

    if (!fake)
        time = get_time ();

    uint32_t modifiers = 0;
    if (key.is_shift_down ())
        modifiers |= MOD_SHIFT_MASK;
    if (key.is_alt_down ())
        modifiers |= MOD_ALT_MASK;
    if (key.is_control_down ())
        modifiers |= MOD_CONTROL_MASK;

    _keyname_to_keysym (key.code, &modifiers);

    if (ic)
        wsc_context_send_key (ic, key.code, modifiers, time, key.is_key_press ());
}

static void
reload_config_callback (const ConfigPointer &config)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    //FIXME:_frontend_hotkey_matcher and _imengine_hotkey_matcher should be added
    //_frontend_hotkey_matcher.load_hotkeys (config);
    //_imengine_hotkey_matcher.load_hotkeys (config);

    KeyEvent key;
    scim_string_to_key (key,
                        config->read (String (SCIM_CONFIG_HOTKEYS_FRONTEND_VALID_KEY_MASK),
                                      String ("Shift+Control+Alt+Lock")));

    _valid_key_mask = (key.mask > 0) ? (key.mask) : 0xFFFF;
    _valid_key_mask |= SCIM_KEY_ReleaseMask;
    // Special treatment for two backslash keys on jp106 keyboard.
    _valid_key_mask |= SCIM_KEY_QuirkKanaRoMask;

    _on_the_spot = config->read (String (SCIM_CONFIG_FRONTEND_ON_THE_SPOT), _on_the_spot);
    //_shared_input_method = config->read (String (SCIM_CONFIG_FRONTEND_SHARED_INPUT_METHOD), _shared_input_method);
    _change_keyboard_mode_by_focus_move = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_CHANGE_KEYBOARD_MODE_BY_FOCUS_MOVE), _change_keyboard_mode_by_focus_move);
    _support_hw_keyboard_mode = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_SUPPORT_HW_KEYBOARD_MODE), _support_hw_keyboard_mode);

    // Get keyboard layout setting
    // Flush the global config first, in order to load the new configs from disk.
    scim_global_config_flush ();

    _keyboard_layout = scim_get_default_keyboard_layout ();
}

class WaylandPanelAgent: public PanelAgentBase
{
    Connection _config_connection;

public:
    WaylandPanelAgent ()
        : PanelAgentBase ("wayland") {
    }
    ~WaylandPanelAgent () {
        stop ();
    }
    bool initialize (InfoManager* info_manager, const String& display, bool resident) {
        LOGD ("");
        g_info_manager = info_manager;
        isf_wsc_context_init ();

        if (!_wsc_setup (&_wsc)) {
            return false;
        }

        _config_connection = _config->signal_connect_reload (slot (reload_config_callback));

        return true;
    }
    bool valid (void) const {
        return true;
    }

    void stop (void) {
        _config_connection.disconnect ();
        isf_wsc_context_shutdown ();
    }

public:
    void
    exit (int id, uint32 contextid) {
        LOGD ("client id:%d", id);
        finalize ();
    }

    void
    update_preedit_caret (int id, uint32 context_id, uint32 caret) {
        LOGD ("client id:%d", id);
        panel_slot_update_preedit_caret (context_id, caret);
    }

    void
    socket_helper_key_event (int id, uint32 context_id,  int cmd , KeyEvent& key) {
        LOGD ("client id:%d", id);

        if (cmd == SCIM_TRANS_CMD_PROCESS_KEY_EVENT)
            panel_slot_process_key_event (context_id, key);
        else
            panel_slot_forward_key_event (context_id, key);
    }

    void
    commit_string (int id, uint32 context_id, const WideString& wstr) {
        LOGD ("client id:%d", id);
        panel_slot_commit_string (context_id, wstr);
    }
#if 0
    void
    request_help (int id, uint32 context_id) {
        LOGD ("client id:%d", id);
        panel_slot_request_help (context_id);
    }


    void
    request_factory_menu (int id, uint32 context_id) {
        LOGD ("client id:%d", id);
        panel_slot_request_factory_menu (context_id);
    }

    void
    change_factory (int id, uint32 context_id, const String& uuid) {
        LOGD ("client id:%d", id);
        panel_slot_change_factory (context_id, uuid);
    }


    void
    reset_keyboard_ise (int id, uint32 context_id) {
        LOGD ("client id:%d", id);
        panel_slot_reset_keyboard_ise (context_id);
    }


    void
    update_keyboard_ise (int id, uint32 context_id) {
        LOGD ("client id:%d", id);
        panel_slot_update_keyboard_ise (context_id);
    }
#endif

    void
    show_preedit_string (int id, uint32 context_id) {
        LOGD ("client id:%d", id);
        _show_preedit_string (context_id);
    }

    void
    hide_preedit_string (int id, uint32 context_id) {
        LOGD ("client id:%d", id);
        _hide_preedit_string (context_id, true);
    }

    void
    update_preedit_string (int id, uint32 context_id, WideString wstr, AttributeList& attrs, uint32 caret) {
        LOGD ("client id:%d", id);
        _update_preedit_string (context_id, wstr, attrs, caret);
    }

    static Eina_Bool
    surrounding_text_fd_read_func(void* data, Ecore_Fd_Handler* fd_handler) {
        if (fd_handler == NULL || data == NULL)
            return ECORE_CALLBACK_RENEW;

        WSCContextISF* wsc_ctx = (WSCContextISF*)data;

        int fd = ecore_main_fd_handler_fd_get(fd_handler);
        char buff[512];
        int len = read (fd, buff, sizeof(buff));
        if (len == 0) {
            LOGD ("update");
            g_info_manager->socket_update_surrounding_text (wsc_ctx->surrounding_text ? wsc_ctx->surrounding_text : "", wsc_ctx->surrounding_cursor);
        } else if (len < 0) {
            LOGW ("failed");
        } else {
            buff[len] = '\0';
            if (wsc_ctx->surrounding_text == NULL) {
                wsc_ctx->surrounding_text = (char*)malloc (len + 1);
                if (wsc_ctx->surrounding_text) {
                    memcpy (wsc_ctx->surrounding_text, buff, len);
                    wsc_ctx->surrounding_text[len] = '\0';
                    return ECORE_CALLBACK_RENEW;
                } else {
                    LOGE ("malloc failed");
                }
            } else {
                int old_len = strlen(wsc_ctx->surrounding_text);
                void * _new = realloc (wsc_ctx->surrounding_text, len + old_len + 1);
                if (_new) {
                    wsc_ctx->surrounding_text = (char*)_new;
                    memcpy (wsc_ctx->surrounding_text + old_len, buff, len);
                    wsc_ctx->surrounding_text[old_len + len] = '\0';
                    return ECORE_CALLBACK_RENEW;
                } else {
                    LOGE ("realloc failed");
                }
            }
        }

        if (wsc_ctx->surrounding_text_fd_read_handler) {
            close(fd);
            ecore_main_fd_handler_del(wsc_ctx->surrounding_text_fd_read_handler);
            wsc_ctx->surrounding_text_fd_read_handler = NULL;
        }
        if (wsc_ctx->surrounding_text) {
            free (wsc_ctx->surrounding_text);
            wsc_ctx->surrounding_text = NULL;
        }
        return ECORE_CALLBACK_RENEW;
    }

    void
    socket_helper_get_surrounding_text (int id, uint32 context_id, uint32 maxlen_before, uint32 maxlen_after) {
        LOGD ("client id:%d", id);
        WSCContextISF* ic = find_ic (context_id);

        int filedes[2];
        if (pipe2(filedes,O_CLOEXEC|O_NONBLOCK) ==-1 ) {
            LOGW ("create pipe failed");
            return;
        }
        LOGD("%d,%d",filedes[0],filedes[1]);
        wl_input_method_context_get_surrounding_text(ic->im_ctx, maxlen_before, maxlen_after, filedes[1]);
        ecore_wl_flush();
        close (filedes[1]);
        if (ic->surrounding_text_fd_read_handler) {
            int fd = ecore_main_fd_handler_fd_get(ic->surrounding_text_fd_read_handler);
            close(fd);
            ecore_main_fd_handler_del(ic->surrounding_text_fd_read_handler);
            ic->surrounding_text_fd_read_handler = NULL;
        }
        if (ic->surrounding_text) {
            free (ic->surrounding_text);
            ic->surrounding_text = NULL;
        }
        ic->surrounding_text_fd_read_handler = ecore_main_fd_handler_add(filedes[0], ECORE_FD_READ, surrounding_text_fd_read_func, ic, NULL, NULL);
    }

    void
    socket_helper_delete_surrounding_text (int id, uint32 context_id, uint32 offset, uint32 len) {
        LOGD ("client id:%d", id);
        //panel_slot_delete_surrounding_text (context_id, offset, len);
        wsc_context_delete_surrounding (_focused_ic, offset, len);
    }

    void
    socket_helper_set_selection (int id, uint32 context_id, uint32 start, uint32 end) {
        LOGD ("client id:%d", id);
        wsc_context_set_selection (_focused_ic, start, end);
    }

    void
    send_private_command (int id, uint32 context_id, const String& command) {
        LOGD ("client id:%d", id);
        //panel_slot_send_private_command (context_id, command);
        wl_input_method_context_private_command (_focused_ic->im_ctx, _focused_ic->serial, command.c_str ());
    }

    void
    reload_config (int id)
    {
        LOGD ("client id:%d", id);
    }

    void
    hide_helper_ise (int id, uint32 context_id)
    {
        LOGD ("client id:%d", id);
        WSCContextISF* ic = find_ic (context_id);

        if (ic) {
            wl_input_method_context_hide_input_panel (ic->im_ctx, ic->serial);
        }
    }

    static Eina_Bool
    selection_text_fd_read_func(void* data, Ecore_Fd_Handler* fd_handler) {
        if (fd_handler == NULL || data == NULL)
            return ECORE_CALLBACK_RENEW;

        WSCContextISF* wsc_ctx = (WSCContextISF*)data;
        LOGD("");
        int fd = ecore_main_fd_handler_fd_get(fd_handler);
        char buff[512];
        int len = read (fd, buff, sizeof(buff));
        if (len == 0) {
            LOGD ("update");
            g_info_manager->socket_update_selection (wsc_ctx->selection_text ? wsc_ctx->selection_text : "");
        } else if (len < 0) {
            LOGW ("failed");
        } else {
            buff[len] = '\0';
            if (wsc_ctx->selection_text == NULL) {
                wsc_ctx->selection_text = (char*)malloc (len + 1);
                if (wsc_ctx->selection_text) {
                    memcpy (wsc_ctx->selection_text, buff, len);
                    wsc_ctx->selection_text[len] = '\0';
                    return ECORE_CALLBACK_RENEW;
                } else {
                    LOGE ("malloc failed");
                }
            } else {
                int old_len = strlen(wsc_ctx->selection_text);
                void * _new = realloc (wsc_ctx->selection_text, len + old_len + 1);
                if (_new) {
                    wsc_ctx->selection_text = (char*)_new;
                    memcpy (wsc_ctx->selection_text + old_len, buff, len);
                    wsc_ctx->selection_text[old_len + len] = '\0';
                    return ECORE_CALLBACK_RENEW;
                } else {
                    LOGE ("realloc failed");
                }
            }
        }

        if (wsc_ctx->selection_text_fd_read_handler) {
            close(fd);
            ecore_main_fd_handler_del(wsc_ctx->selection_text_fd_read_handler);
            wsc_ctx->selection_text_fd_read_handler = NULL;
        }
        if (wsc_ctx->selection_text) {
            free (wsc_ctx->selection_text);
            wsc_ctx->selection_text = NULL;
        }
        return ECORE_CALLBACK_RENEW;
    }

    void
    socket_helper_get_selection (int id, uint32 context_id) {
        LOGD ("client id:%d", id);
        WSCContextISF* ic = find_ic (context_id);

        int filedes[2];
        if (pipe2(filedes,O_CLOEXEC|O_NONBLOCK) ==-1 ) {
            LOGW ("create pipe failed");
            return;
        }
        LOGD("%d,%d",filedes[0],filedes[1]);
        wl_input_method_context_get_selection_text(ic->im_ctx, filedes[1]);
        ecore_wl_flush();
        close (filedes[1]);
        if (ic->selection_text_fd_read_handler) {
            int fd = ecore_main_fd_handler_fd_get(ic->selection_text_fd_read_handler);
            close(fd);
            ecore_main_fd_handler_del(ic->selection_text_fd_read_handler);
            ic->selection_text_fd_read_handler = NULL;
        }
        if (ic->selection_text) {
            free (ic->selection_text);
            ic->selection_text = NULL;
        }
        ic->selection_text_fd_read_handler = ecore_main_fd_handler_add(filedes[0], ECORE_FD_READ, selection_text_fd_read_func, ic, NULL, NULL);
    }

    void process_key_event_done(int id, uint32 context_id, KeyEvent &key, uint32 ret, uint32 serial) {
        LOGD ("client id:%d", id);
        WSCContextISF* ic = find_ic (context_id);
        if (ret == EINA_FALSE) {
            if (key.is_key_press()) {
                if (key.code == SCIM_KEY_space ||
                    key.code == SCIM_KEY_KP_Space)
                    autoperiod_insert (ic);
            }
        }

        if (ret == EINA_FALSE) {
            send_wl_key_event (ic, key, false);
        }
    }
};

static scim::PanelAgentPointer instance;
extern "C" {

    EXAPI void scim_module_init (void)
    {
        LOGD ("");
    }

    EXAPI void scim_module_exit (void)
    {
        LOGD ("");
        instance.reset();
    }

    EXAPI void scim_panel_agent_module_init (const scim::ConfigPointer& config)
    {
        LOGD ("");
        _config = config;
    }

    EXAPI scim::PanelAgentPointer scim_panel_agent_module_get_instance ()
    {
        scim::PanelAgentBase* _instance = NULL;
        if (instance.null()) {
            try {
                _instance = new WaylandPanelAgent();
            } catch (...) {
                delete _instance;
                _instance = NULL;
            }
            if(_instance)
                instance = _instance;
        }
        return instance;

    }
}

/*
vi:ts=4:nowrap:expandtab
*/
