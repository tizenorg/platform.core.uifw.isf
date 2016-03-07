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

#define Uses_SCIM_BACKEND
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_HELPER_MODULE
#define Uses_SCIM_HOTKEY
#define Uses_SCIM_PANEL_CLIENT
#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_PANEL_AGENT
#define Uses_SCIM_COMPOSE_KEY

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/times.h>
#include <pthread.h>
#include <langinfo.h>
#include <unistd.h>

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <dlog.h>

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


#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG                                         "ISF_WAYLAND_MODULE"

using namespace scim;

struct _WSCContextISFImpl {
    WSCContextISF           *parent;
    IMEngineInstancePointer  si;
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
    bool                     shared_si;
    bool                     preedit_started;
    bool                     preedit_updating;
    bool                     need_commit_preedit;
    bool                     prediction_allow;
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
                           shared_si(false),
                           preedit_started(false),
                           preedit_updating(false),
                           need_commit_preedit(false),
                           prediction_allow(false),
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
//static void     panel_slot_reload_config                (int                     context);
static void     panel_slot_exit                         (int                     context);
static void     panel_slot_update_candidate_item_layout (int                     context,
                                                         const std::vector<uint32> &row_items);
static void     panel_slot_update_lookup_table_page_size(int                     context,
                                                         int                     page_size);
static void     panel_slot_lookup_table_page_up         (int                     context);
static void     panel_slot_lookup_table_page_down       (int                     context);
static void     panel_slot_trigger_property             (int                     context,
                                                         const String           &property);
static void     panel_slot_process_helper_event         (int                     context,
                                                         const String           &target_uuid,
                                                         const String           &helper_uuid,
                                                         const Transaction      &trans);
//static void     panel_slot_move_preedit_caret           (int                     context,
//                                                         int                     caret_pos);
static void     panel_slot_update_preedit_caret         (int                     context,
                                                         int                     caret);
static void     panel_slot_select_aux                   (int                     context,
                                                         int                     aux_index);
static void     panel_slot_select_candidate             (int                     context,
                                                         int                     cand_index);
static void     panel_slot_process_key_event            (int                     context,
                                                         const KeyEvent         &key);
static void     panel_slot_commit_string                (int                     context,
                                                         const WideString       &wstr);
static void     panel_slot_forward_key_event            (int                     context,
                                                         const KeyEvent         &key);
//static void     panel_slot_request_help                 (int                     context);
//static void     panel_slot_request_factory_menu         (int                     context);
static void     panel_slot_change_factory               (int                     context,
                                                         const String           &uuid);
static void     panel_slot_reset_keyboard_ise           (int                     context);
static void     panel_slot_update_keyboard_ise          (int                     context);
static void     panel_slot_show_preedit_string          (int                     context);
static void     panel_slot_hide_preedit_string          (int                     context);
static void     panel_slot_update_preedit_string        (int                     context,
                                                         const WideString       &str,
                                                         const AttributeList    &attrs,
                                                         int               caret);
static void     panel_slot_get_surrounding_text         (int                     context,
                                                         int                     maxlen_before,
                                                         int                     maxlen_after);
static void     panel_slot_delete_surrounding_text      (int                     context,
                                                         int                     offset,
                                                         int                     len);

static void     panel_slot_set_selection                (int                     context,
                                                         int                     start,
                                                         int                     end);
static void     panel_slot_send_private_command         (int                     context,
                                                         const String           &command);
static void     panel_req_focus_in                      (WSCContextISF     *ic);
static void     panel_req_update_factory_info           (WSCContextISF     *ic);
static void     panel_req_update_cursor_position        (WSCContextISF     *ic, int cursor_pos);
static void     panel_req_update_bidi_direction         (WSCContextISF     *ic, int direction);
//static void     panel_req_show_help                     (WSCContextISF     *ic);
static void     panel_req_show_factory_menu             (WSCContextISF     *ic);

/* Panel iochannel handler*/
static bool     panel_initialize                        (void);
static void     panel_finalize                          (void);


/* utility functions */
static bool     filter_hotkeys                          (WSCContextISF     *ic,
                                                         const KeyEvent         &key);
static bool     filter_keys                             (const char *keyname,
                                                         const char *config_path);

static void     turn_on_ic                              (WSCContextISF     *ic);
static void     turn_off_ic                             (WSCContextISF     *ic);
static void     set_ic_capabilities                     (WSCContextISF     *ic);

static void     initialize                              (void);
static void     finalize                                (void);

static void     open_next_factory                       (WSCContextISF     *ic);
static void     open_previous_factory                   (WSCContextISF     *ic);
static void     open_specific_factory                   (WSCContextISF     *ic,
                                                         const String           &uuid);
static void     send_wl_key_event                       (WSCContextISF *ic, const KeyEvent &key, bool fake);
static void     _hide_preedit_string                    (int context, bool update_preedit);

static void     attach_instance                         (const IMEngineInstancePointer &si);

/* slot functions */
static void     slot_show_preedit_string                (IMEngineInstanceBase   *si);
static void     slot_show_aux_string                    (IMEngineInstanceBase   *si);
static void     slot_show_lookup_table                  (IMEngineInstanceBase   *si);

static void     slot_hide_preedit_string                (IMEngineInstanceBase   *si);
static void     slot_hide_aux_string                    (IMEngineInstanceBase   *si);
static void     slot_hide_lookup_table                  (IMEngineInstanceBase   *si);

static void     slot_update_preedit_caret               (IMEngineInstanceBase   *si,
                                                         int                     caret);
static void     slot_update_preedit_string              (IMEngineInstanceBase   *si,
                                                         const WideString       &str,
                                                         const AttributeList    &attrs,
                                                         int               caret);
static void     slot_update_aux_string                  (IMEngineInstanceBase   *si,
                                                         const WideString       &str,
                                                         const AttributeList    &attrs);
static void     slot_commit_string                      (IMEngineInstanceBase   *si,
                                                         const WideString       &str);
static void     slot_forward_key_event                  (IMEngineInstanceBase   *si,
                                                         const KeyEvent         &key);
static void     slot_update_lookup_table                (IMEngineInstanceBase   *si,
                                                         const LookupTable      &table);

static void     slot_register_properties                (IMEngineInstanceBase   *si,
                                                         const PropertyList     &properties);
static void     slot_update_property                    (IMEngineInstanceBase   *si,
                                                         const Property         &property);
static void     slot_beep                               (IMEngineInstanceBase   *si);
static void     slot_start_helper                       (IMEngineInstanceBase   *si,
                                                         const String           &helper_uuid);
static void     slot_stop_helper                        (IMEngineInstanceBase   *si,
                                                         const String           &helper_uuid);
static void     slot_send_helper_event                  (IMEngineInstanceBase   *si,
                                                         const String           &helper_uuid,
                                                         const Transaction      &trans);
static bool     slot_get_surrounding_text               (IMEngineInstanceBase   *si,
                                                         WideString             &text,
                                                         int                    &cursor,
                                                         int                     maxlen_before,
                                                         int                     maxlen_after);
static bool     slot_delete_surrounding_text            (IMEngineInstanceBase   *si,
                                                         int                     offset,
                                                         int                     len);
static bool     slot_set_selection                      (IMEngineInstanceBase   *si,
                                                         int                     start,
                                                         int                     end);

static void     slot_expand_candidate                   (IMEngineInstanceBase   *si);
static void     slot_contract_candidate                 (IMEngineInstanceBase   *si);

static void     slot_set_candidate_style                (IMEngineInstanceBase   *si,
                                                         ISF_CANDIDATE_PORTRAIT_LINE_T portrait_line,
                                                         ISF_CANDIDATE_MODE_T    mode);

static void     slot_send_private_command               (IMEngineInstanceBase   *si,
                                                         const String           &command);

//static void     reload_config_callback                  (const ConfigPointer    &config);

static void     fallback_commit_string_cb               (IMEngineInstanceBase   *si,
                                                         const WideString       &str);
static void     _display_input_language                 (WSCContextISF *ic);

/* Local variables declaration */
static String                                           _language;
static WSCContextISFImpl                               *_used_ic_impl_list          = 0;
static WSCContextISFImpl                               *_free_ic_impl_list          = 0;
static WSCContextISF                                   *_ic_list                    = 0;

static KeyboardLayout                                   _keyboard_layout            = SCIM_KEYBOARD_Default;
static int                                              _valid_key_mask             = SCIM_KEY_AllMasks;

static FrontEndHotkeyMatcher                            _frontend_hotkey_matcher;
static IMEngineHotkeyMatcher                            _imengine_hotkey_matcher;

static IMEngineInstancePointer                          _default_instance;

static ConfigPointer                                    _config;
static BackEndPointer                                   _backend;

static WSCContextISF                                   *_focused_ic                 = 0;

static bool                                             _scim_initialized           = false;

static int                                              _instance_count             = 0;

static IMEngineFactoryPointer                           _fallback_factory;
static IMEngineInstancePointer                          _fallback_instance;
static int                                              _panel_client_id            = 0;
static uint32                                           _active_helper_option       = 0;


static bool                                             _on_the_spot                = true;
static bool                                             _shared_input_method        = false;
static bool                                             _change_keyboard_mode_by_focus_move = false;
static bool                                             _support_hw_keyboard_mode   = false;
static double                                           space_key_time              = 0.0;

static Eina_Bool                                        autoperiod_allow            = EINA_FALSE;
static Eina_Bool                                        autocap_allow               = EINA_FALSE;

static bool                                             _x_key_event_is_valid       = false;

static Input_Language                                   input_lang                  = INPUT_LANG_OTHER;

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
_wsc_im_ctx_surrounding_text(void *data, struct wl_input_method_context *im_ctx, const char *text, uint32_t cursor, uint32_t anchor)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    if (!wsc_ctx) return;

    if (wsc_ctx->surrounding_text)
        free (wsc_ctx->surrounding_text);

    wsc_ctx->surrounding_text = strdup (text ? text : "");
    wsc_ctx->surrounding_cursor = cursor;

    isf_wsc_context_cursor_position_set(wsc_ctx, cursor);

    LOGD ("text : '%s', cursor : %d\n", text, cursor);
}

static void
_wsc_im_ctx_reset(void *data, struct wl_input_method_context *im_ctx)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    if (!wsc_ctx) return;
    isf_wsc_context_reset(wsc_ctx);
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

    if (wsc_ctx->surrounding_text)
        LOGD ("Surrounding text updated: '%s'\n", wsc_ctx->surrounding_text);

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

static const struct wl_input_method_context_listener wsc_im_context_listener = {
     _wsc_im_ctx_surrounding_text,
     _wsc_im_ctx_reset,
     _wsc_im_ctx_content_type,
     _wsc_im_ctx_invoke_action,
     _wsc_im_ctx_commit_state,
     _wsc_im_ctx_preferred_language,
     _wsc_im_ctx_return_key_type,
     _wsc_im_ctx_return_key_disabled,
     _wsc_im_ctx_input_panel_data,
     _wsc_im_ctx_bidi_direction
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

    LOGW ("create _keysym2keycode\n");
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

    int len = 1024;
    char imdata[1024] = {0};
    isf_wsc_context_input_panel_imdata_get (wsc_ctx, imdata, &len);
    LOGD ("Get imdata:%s, length:%d\n", imdata, len);
    wl_input_method_context_update_input_panel_data (im_ctx, wsc_ctx->serial, imdata, len);
}

static void
_wsc_im_deactivate(void *data, struct wl_input_method *input_method, struct wl_input_method_context *im_ctx)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc || !wsc->wsc_ctx) return;
    WSCContextISF *wsc_ctx = wsc->wsc_ctx;

    isf_wsc_context_input_panel_hide (wsc_ctx);
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

    if (!(registry = ecore_wl_registry_get()))
        return false;

    if (!(globals = ecore_wl_globals_get()))
        return false;

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
            rec->si.reset ();
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

static void
set_prediction_allow (WSCContextISF *ctx, bool prediction)
{
    WSCContextISF *context_scim = ctx;

    if (context_scim && context_scim->impl && context_scim->impl->si && context_scim == _focused_ic) {
        context_scim->impl->si->set_prediction_allow (prediction);
    }
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

void
imengine_layout_set (WSCContextISF *wsc_ctx, Ecore_IMF_Input_Panel_Layout layout)
{
    LOGD ("");
    WSCContextISF* context_scim = wsc_ctx;

    if (context_scim && context_scim->impl && context_scim->impl->si && context_scim == _focused_ic) {
        context_scim->impl->si->set_layout (layout);
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

    context_scim->impl = NULL;

    if (_backend.null ())
        return;

    IMEngineInstancePointer si;

    // Use the default instance if "shared input method" mode is enabled.
    if (_shared_input_method && !_default_instance.null ()) {
        si = _default_instance;
        SCIM_DEBUG_FRONTEND(2) << "use default instance: " << si->get_id () << " " << si->get_factory_uuid () << "\n";
    }

    // Not in "shared input method" mode, or no default instance, create an instance.
    if (si.null ()) {
        IMEngineFactoryPointer factory = _backend->get_default_factory (_language, "UTF-8");
        if (factory.null ()) return;
        si = factory->create_instance ("UTF-8", _instance_count++);
        if (si.null ()) return;
        attach_instance (si);
        SCIM_DEBUG_FRONTEND(2) << "create new instance: " << si->get_id () << " " << si->get_factory_uuid () << "\n";
    }

    // If "shared input method" mode is enabled, and there is no default instance,
    // then store this instance as default one.
    if (_shared_input_method && _default_instance.null ()) {
        SCIM_DEBUG_FRONTEND(2) << "update default instance.\n";
        _default_instance = si;
    }

    context_scim->impl                      = new_ic_impl (context_scim);
    if (context_scim->impl == NULL) {
        std::cerr << "memory allocation failed in " << __FUNCTION__ << "\n";
        return;
    }

    context_scim->impl->si                  = si;
    context_scim->impl->client_window       = 0;
    context_scim->impl->preedit_caret       = 0;
    context_scim->impl->cursor_x            = 0;
    context_scim->impl->cursor_y            = 0;
    context_scim->impl->cursor_pos          = -1;
    context_scim->impl->cursor_top_y        = 0;
    context_scim->impl->is_on               = true;
    context_scim->impl->shared_si           = _shared_input_method;
    context_scim->impl->use_preedit         = _on_the_spot;
    context_scim->impl->preedit_started     = false;
    context_scim->impl->preedit_updating    = false;
    context_scim->impl->need_commit_preedit = false;
    context_scim->impl->prediction_allow    = true;

    if (!_ic_list)
        context_scim->next = NULL;
    else
        context_scim->next = _ic_list;
    _ic_list = context_scim;

    if (_shared_input_method)
        context_scim->impl->is_on = _config->read (String (SCIM_CONFIG_FRONTEND_IM_OPENED_BY_DEFAULT), context_scim->impl->is_on);

    g_info_manager->register_input_context (WAYLAND_MODULE_CLIENT_ID, context_scim->id, si->get_factory_uuid ());
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
        if (context_scim == _focused_ic && context_scim->impl->si)
            context_scim->impl->si->focus_out ();

        // Delete the instance.
        // FIXME:
        // In case the instance send out some helper event,
        // and this context has been focused out,
        // we need set the focused_ic to this context temporary.
        WSCContextISF* old_focused = _focused_ic;
        _focused_ic = context_scim;
        context_scim->impl->si.reset ();
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

    bool need_cap   = false;
    bool need_reset = false;
    bool need_reg   = false;

    if (context_scim && context_scim->impl) {
        _focused_ic = context_scim;

        // Handle the "Shared Input Method" mode.
        if (_shared_input_method) {
            SCIM_DEBUG_FRONTEND (2) << "shared input method.\n";
            IMEngineFactoryPointer factory = _backend->get_default_factory (_language, "UTF-8");

            if (!factory.null ()) {
                if (_default_instance.null () || _default_instance->get_factory_uuid () != factory->get_uuid ()) {
                    _default_instance = factory->create_instance ("UTF-8", _default_instance.null () ? _instance_count++ : _default_instance->get_id ());
                    attach_instance (_default_instance);
                    SCIM_DEBUG_FRONTEND (2) << "create new default instance: " << _default_instance->get_id () << " " << _default_instance->get_factory_uuid () << "\n";
                }

                context_scim->impl->shared_si = true;
                context_scim->impl->si = _default_instance;
                context_scim->impl->is_on = _config->read (String (SCIM_CONFIG_FRONTEND_IM_OPENED_BY_DEFAULT), context_scim->impl->is_on);
                context_scim->impl->preedit_string.clear ();
                context_scim->impl->preedit_attrlist.clear ();
                context_scim->impl->preedit_caret = 0;
                context_scim->impl->preedit_started = false;
                need_cap = true;
                need_reset = true;
                need_reg = true;
            }
        } else if (context_scim->impl->shared_si) {
            SCIM_DEBUG_FRONTEND (2) << "exit shared input method.\n";
            IMEngineFactoryPointer factory = _backend->get_default_factory (_language, "UTF-8");

            if (!factory.null ()) {
                context_scim->impl->si = factory->create_instance ("UTF-8", _instance_count++);
                context_scim->impl->preedit_string.clear ();
                context_scim->impl->preedit_attrlist.clear ();
                context_scim->impl->preedit_caret = 0;
                context_scim->impl->preedit_started = false;
                attach_instance (context_scim->impl->si);
                need_cap = true;
                need_reg = true;
                context_scim->impl->shared_si = false;
                SCIM_DEBUG_FRONTEND (2) << "create new instance: " << context_scim->impl->si->get_id () << " " << context_scim->impl->si->get_factory_uuid () << "\n";
            }
        }

        if (context_scim->impl->si) {
            context_scim->impl->si->set_frontend_data (static_cast <void*> (context_scim));

            if (need_reg) g_info_manager->register_input_context (WAYLAND_MODULE_CLIENT_ID, context_scim->id, context_scim->impl->si->get_factory_uuid ());

            if (need_cap) set_ic_capabilities (context_scim);

            panel_req_focus_in (context_scim);
            //        panel_req_update_factory_info (context_scim);

            if (need_reset) context_scim->impl->si->reset ();

            if (context_scim->impl->is_on) {
                g_info_manager->socket_turn_on ();
                //            _panel_client.hide_preedit_string (context_scim->id);
                //            _panel_client.hide_aux_string (context_scim->id);
                //            _panel_client.hide_lookup_table (context_scim->id);
                context_scim->impl->si->focus_in ();
                context_scim->impl->si->set_layout (wsc_context_input_panel_layout_get (wsc_ctx));
                context_scim->impl->si->set_prediction_allow (context_scim->impl->prediction_allow);

                if (context_scim->impl->imdata)
                    context_scim->impl->si->set_imdata ((const char*)context_scim->impl->imdata, context_scim->impl->imdata_size);
            } else {
                g_info_manager->socket_turn_off ();
            }

            g_info_manager->get_active_helper_option (WAYLAND_MODULE_CLIENT_ID, _active_helper_option);

            if (caps_mode_check (wsc_ctx, EINA_FALSE, EINA_TRUE) == EINA_FALSE) {
                context_scim->impl->next_shift_status = 0;
            }
        }
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

        if (context_scim->impl->si) {
            context_scim->impl->si->focus_out ();
            context_scim->impl->si->reset ();
            context_scim->impl->cursor_pos = -1;
//          if (context_scim->impl->shared_si) context_scim->impl->si->reset ();
            g_info_manager->focus_out (WAYLAND_MODULE_CLIENT_ID, context_scim->id);
        }

        _focused_ic = 0;
    }
    _x_key_event_is_valid = false;
}

void
isf_wsc_context_reset (WSCContextISF *wsc_ctx)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *context_scim = wsc_ctx;

    if (context_scim && context_scim->impl && context_scim == _focused_ic) {
        if (context_scim->impl->si) {
            context_scim->impl->si->reset ();
            g_info_manager->socket_reset_input_context (WAYLAND_MODULE_CLIENT_ID, context_scim->id);
        }

        if (context_scim->impl->need_commit_preedit) {
            _hide_preedit_string (context_scim->id, false);
            wsc_context_commit_preedit_string (context_scim);
        }
    }
}

void
isf_wsc_context_cursor_position_set (WSCContextISF *wsc_ctx, int cursor_pos)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* context_scim = wsc_ctx;

    if (context_scim && context_scim->impl && context_scim == _focused_ic) {
        if (context_scim->impl->cursor_pos != cursor_pos) {
            LOGD ("ctx : %p, cursor pos : %d\n", wsc_ctx, cursor_pos);
            context_scim->impl->cursor_pos = cursor_pos;

            caps_mode_check (wsc_ctx, EINA_FALSE, EINA_TRUE);

            if (context_scim->impl->preedit_updating)
                return;

            if (context_scim->impl->si) {
                context_scim->impl->si->update_cursor_position (cursor_pos);
                panel_req_update_cursor_position (context_scim, cursor_pos);
            }
        }
    }
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
isf_wsc_context_prediction_allow_set (WSCContextISF* wsc_ctx, Eina_Bool prediction)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " = " << (prediction == EINA_TRUE ? "true" : "false") << "...\n";
    LOGD ("");
    WSCContextISF* context_scim = wsc_ctx;

    if (context_scim && context_scim->impl && context_scim->impl->prediction_allow != prediction) {
        context_scim->impl->prediction_allow = prediction;
        set_prediction_allow (wsc_ctx, prediction);
    }
}

Eina_Bool
isf_wsc_context_prediction_allow_get (WSCContextISF* wsc_ctx)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* context_scim = wsc_ctx;
    Eina_Bool ret = EINA_FALSE;
    if (context_scim && context_scim->impl) {
        ret = context_scim->impl->prediction_allow;
    } else {
        std::cerr << __FUNCTION__ << " failed!!!\n";
    }
    return ret;
}

void
isf_wsc_context_autocapital_type_set (WSCContextISF* wsc_ctx, Ecore_IMF_Autocapital_Type autocapital_type)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " = " << autocapital_type << "...\n";
    LOGD ("");
    WSCContextISF* context_scim = wsc_ctx;

    if (context_scim && context_scim->impl && context_scim->impl->autocapital_type != autocapital_type) {
        context_scim->impl->autocapital_type = autocapital_type;

        if (context_scim->impl->si && context_scim == _focused_ic) {
            LOGD ("ctx : %p. set autocapital type : %d\n", wsc_ctx, autocapital_type);
            context_scim->impl->si->set_autocapital_type (autocapital_type);
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

            if (context_scim->impl->si && context_scim == _focused_ic) {
                LOGD ("ctx : %p, bidi direction : %#x\n", wsc_ctx, direction);
                context_scim->impl->si->update_bidi_direction (direction);
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

    Eina_Bool ret = EINA_FALSE;
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
        ret = EINA_TRUE;

        if (!filter_hotkeys (wsc_ctx, key)) {
            if (!_focused_ic || !_focused_ic->impl || !_focused_ic->impl->is_on) {
                ret = EINA_FALSE;
#ifdef _TV
            } else if (get_keyboard_mode () == TOOLBAR_HELPER_MODE
                       && _active_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
                uint32 _ret;
                g_info_manager->process_key_event (key, _ret);
                ret = (Eina_Bool)_ret;

                if (!ret) {
                    if (_focused_ic->impl->si)
                        ret = _focused_ic->impl->si->process_key_event (key);
                    else
                        ret = EINA_FALSE;
                }
#else
            } else if (get_keyboard_mode () == TOOLBAR_HELPER_MODE
                       && _active_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
                uint32 _ret;
                g_info_manager->process_key_event (key, _ret);
                ret = (Eina_Bool)_ret;

                if (!ret && ! (_active_helper_option & ISM_HELPER_WITHOUT_IMENGINE)) {
                    if (_focused_ic->impl->si)
                        ret = _focused_ic->impl->si->process_key_event (key);
                    else
                        ret = EINA_FALSE;
                }
#endif
            } else {
                if (_focused_ic->impl->si)
                    ret = _focused_ic->impl->si->process_key_event (key);
                else
                    ret = EINA_FALSE;
            }

            if (ret == EINA_FALSE) {
                if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
                    if (key.code == SCIM_KEY_space ||
                        key.code == SCIM_KEY_KP_Space)
                        autoperiod_insert (wsc_ctx);
                }
            }
        }
    }

    if (ret == EINA_FALSE) {
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

    if (wsc_ctx->preedit_style)
        wl_input_method_context_preedit_styling (wsc_ctx->im_ctx,
                                                 0,
                                                 strlen (wsc_ctx->preedit_str),
                                                 wsc_ctx->preedit_style);
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
    LOGD ("");

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

    if (!wsc_ctx)
        return;

    wsc_ctx->modifiers = modifiers;

    wl_input_method_context_keysym (wsc_ctx->im_ctx, wsc_ctx->serial, time,
            keysym, press ? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED, wsc_ctx->modifiers);
}

static void
turn_on_ic (WSCContextISF *ic)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");

    if (ic && ic->impl && ic->impl->si && !ic->impl->is_on) {
        ic->impl->is_on = true;

        if (ic == _focused_ic) {
            panel_req_focus_in (ic);
            panel_req_update_factory_info (ic);
            g_info_manager->socket_turn_on ();
//            _panel_client.hide_preedit_string (ic->id);
//            _panel_client.hide_aux_string (ic->id);
//            _panel_client.hide_lookup_table (ic->id);
            ic->impl->si->focus_in ();
            ic->impl->si->set_layout (ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL);
            ic->impl->si->set_prediction_allow (ic->impl->prediction_allow);
        }

        //Record the IC on/off status
        if (_shared_input_method) {
            _config->write (String (SCIM_CONFIG_FRONTEND_IM_OPENED_BY_DEFAULT), true);
            _config->flush ();
        }

        if (ic->impl->use_preedit && ic->impl->preedit_string.length ()) {
            if (!check_valid_ic (ic))
                return;

            if (check_valid_ic (ic))
                ic->impl->preedit_started = true;
        }
    }
}

static void
turn_off_ic (WSCContextISF *ic)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");

    if (ic && ic->impl && ic->impl->si && ic->impl->is_on) {
        ic->impl->is_on = false;

        if (ic == _focused_ic) {
            ic->impl->si->focus_out ();

//            panel_req_update_factory_info (ic);
            g_info_manager->socket_turn_off ();
        }

        //Record the IC on/off status
        if (_shared_input_method) {
            _config->write (String (SCIM_CONFIG_FRONTEND_IM_OPENED_BY_DEFAULT), false);
            _config->flush ();
        }

        if (ic->impl->use_preedit && ic->impl->preedit_string.length ()) {
            if (!check_valid_ic (ic))
                return;

            if (check_valid_ic (ic))
                ic->impl->preedit_started = false;
        }
    }
}

static void
set_ic_capabilities (WSCContextISF *ic)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");

    if (ic && ic->impl) {
        unsigned int cap = SCIM_CLIENT_CAP_ALL_CAPABILITIES;

        if (!_on_the_spot || !ic->impl->use_preedit)
            cap -= SCIM_CLIENT_CAP_ONTHESPOT_PREEDIT;

        if (ic->impl->si)
            ic->impl->si->update_client_capabilities (cap);
    }
}

static bool
check_socket_frontend (void)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    SocketAddress address;
    SocketClient client;

    uint32 magic;

    address.set_address (scim_get_default_socket_frontend_address ());

    if (!client.connect (address))
        return false;

    if (!scim_socket_open_connection (magic,
                                      String ("ConnectionTester"),
                                      String ("SocketFrontEnd"),
                                      client,
                                      1000)) {
        return false;
    }

    return true;
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
panel_req_show_factory_menu (WSCContextISF *ic)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    std::vector<IMEngineFactoryPointer> factories;
    std::vector <PanelFactoryInfo> menu;

    _backend->get_factories_for_encoding (factories, "UTF-8");

    for (size_t i = 0; i < factories.size (); ++ i) {
        menu.push_back (PanelFactoryInfo (
                            factories [i]->get_uuid (),
                            utf8_wcstombs (factories [i]->get_name ()),
                            factories [i]->get_language (),
                            factories [i]->get_icon_file ()));
    }

    if (menu.size ())
        g_info_manager->socket_show_factory_menu (menu);
}

static void
panel_req_update_factory_info (WSCContextISF *ic)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");

    if (ic && ic->impl && ic == _focused_ic) {
        PanelFactoryInfo info;
        if (ic->impl->is_on) {
            IMEngineFactoryPointer sf = _backend->get_factory (ic->impl->si->get_factory_uuid ());
            if (sf)
                info = PanelFactoryInfo (sf->get_uuid (), utf8_wcstombs (sf->get_name ()), sf->get_language (), sf->get_icon_file ());
        } else {
            info = PanelFactoryInfo (String (""), String (_("English Keyboard")), String ("C"), String (SCIM_KEYBOARD_ICON_FILE));
        }

        g_info_manager->socket_update_factory_info (info);
    }
}

static void
panel_req_focus_in (WSCContextISF *ic)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");

    if (ic && ic->impl && ic->impl->si)
        g_info_manager->focus_in (WAYLAND_MODULE_CLIENT_ID, ic->id, ic->impl->si->get_factory_uuid ());
}

static void
panel_req_update_cursor_position (WSCContextISF *ic, int cursor_pos)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");

    if (ic)
        g_info_manager->socket_update_cursor_position (cursor_pos);
}

static void
panel_req_update_bidi_direction   (WSCContextISF *ic, int direction)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic)
        g_info_manager->update_ise_bidi_direction (WAYLAND_MODULE_CLIENT_ID, direction);
}

static bool
filter_hotkeys (WSCContextISF *ic, const KeyEvent &key)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    bool ret = false;

    if (!check_valid_ic (ic))
        return ret;

    _frontend_hotkey_matcher.push_key_event (key);
    _imengine_hotkey_matcher.push_key_event (key);

    FrontEndHotkeyAction hotkey_action = _frontend_hotkey_matcher.get_match_result ();

    if (hotkey_action == SCIM_FRONTEND_HOTKEY_TRIGGER) {
        IMEngineFactoryPointer sf = _backend->get_factory (ic->impl->si->get_factory_uuid ());
        if (sf && (sf->get_option () & SCIM_IME_SUPPORT_LANGUAGE_TOGGLE_KEY)) {
            ret = false;
        } else {
            if (!ic->impl->is_on) {
                turn_on_ic (ic);
            } else {
                turn_off_ic (ic);
                g_info_manager->socket_hide_aux_string ();
                g_info_manager->socket_hide_lookup_table ();
            }

            _display_input_language (ic);
            ret = true;
        }
    } else if (hotkey_action == SCIM_FRONTEND_HOTKEY_ON) {
        if (!ic->impl->is_on) {
            turn_on_ic (ic);
            _display_input_language (ic);
        }
        ret = true;
    } else if (hotkey_action == SCIM_FRONTEND_HOTKEY_OFF) {
        if (ic->impl->is_on) {
            turn_off_ic (ic);
            _display_input_language (ic);
        }
        ret = true;
    } else if (hotkey_action == SCIM_FRONTEND_HOTKEY_NEXT_FACTORY) {
        open_next_factory (ic);
        ret = true;
    } else if (hotkey_action == SCIM_FRONTEND_HOTKEY_PREVIOUS_FACTORY) {
        open_previous_factory (ic);
        ret = true;
    } else if (hotkey_action == SCIM_FRONTEND_HOTKEY_SHOW_FACTORY_MENU) {
        panel_req_show_factory_menu (ic);
        ret = true;
    } else if (_imengine_hotkey_matcher.is_matched ()) {
        ISEInfo info = _imengine_hotkey_matcher.get_match_result ();
        ISE_TYPE type = info.type;
        if (type == IMENGINE_T)
            open_specific_factory (ic, info.uuid);
        else if (type == HELPER_T)
            g_info_manager->start_helper (info.uuid);

        ret = true;
    }
    return ret;
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

static bool
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
        if (context_scim->impl && context_scim->impl->si) {
            g_info_manager->register_input_context (WAYLAND_MODULE_CLIENT_ID, context_scim->id, context_scim->impl->si->get_factory_uuid ());
        }

        context_scim = context_scim->next;
    }

    if (_focused_ic) {
        _focused_ic = 0;
    }

    return true;
}

static void
panel_finalize (void)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    g_info_manager->del_client (WAYLAND_MODULE_CLIENT_ID);
}

static void
panel_slot_exit (int /* context */)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    finalize ();
}

static void
panel_slot_update_candidate_item_layout (int context, const std::vector<uint32> &row_items)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " row size=" << row_items.size () << " ic=" << ic << "\n";

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        ic->impl->si->update_candidate_item_layout (row_items);
    }
}

static void
panel_slot_update_lookup_table_page_size (int context, int page_size)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " page_size=" << page_size << " ic=" << ic << "\n";

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        ic->impl->si->update_lookup_table_page_size (page_size);
    }
}

static void
panel_slot_lookup_table_page_up (int context)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        ic->impl->si->lookup_table_page_up ();
    }
}

static void
panel_slot_lookup_table_page_down (int context)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        ic->impl->si->lookup_table_page_down ();
    }
}

static void
panel_slot_trigger_property (int context, const String &property)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " property=" << property << " ic=" << ic << "\n";

    if (ic && ic->impl && ic->impl->si) {
        ic->impl->si->trigger_property (property);
    }
}

static void
panel_slot_process_helper_event (int context, const String &target_uuid, const String &helper_uuid, const Transaction &trans)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " target=" << target_uuid
                            << " helper=" << helper_uuid << " ic=" << ic << " ic->impl=" << (ic != NULL ? ic->impl : 0) << " ic-uuid="
                            << ((ic && ic->impl && ic->impl->si) ? ic->impl->si->get_factory_uuid () : "") << " _focused_ic=" << _focused_ic << "\n";

    if (ic && ic->impl && _focused_ic == ic && ic->impl->is_on && ic->impl->si &&
        ic->impl->si->get_factory_uuid () == target_uuid) {
        SCIM_DEBUG_FRONTEND (2) << "call process_helper_event\n";
        ic->impl->si->process_helper_event (helper_uuid, trans);
    }
}

//useless
#if 0
static void
panel_slot_move_preedit_caret (int context, int caret_pos)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " caret=" << caret_pos << " ic=" << ic << "\n";

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        ic->impl->si->move_preedit_caret (caret_pos);
    }
}
#endif

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

static void
panel_slot_select_aux (int context, int aux_index)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " aux=" << aux_index << " ic=" << ic << "\n";

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        ic->impl->si->select_aux (aux_index);
    }
}

static void
panel_slot_select_candidate (int context, int cand_index)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " candidate=" << cand_index << " ic=" << ic << "\n";

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        ic->impl->si->select_candidate (cand_index);
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
    Eina_Bool process_key = EINA_TRUE;
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " key=" << key.get_key_string () << " ic=" << ic << "\n";

    if (!(ic && ic->impl))
        return;

    if ((_focused_ic != NULL) && (_focused_ic != ic))
        return;

    KeyEvent _key = key;
    if (key.is_key_press () &&
        wsc_context_input_panel_layout_get (ic) == ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL) {
        if (key.code == SHIFT_MODE_OFF ||
            key.code == SHIFT_MODE_ON ||
            key.code == SHIFT_MODE_LOCK) {
            ic->impl->next_shift_status = _key.code;
        } else if (key.code == SHIFT_MODE_ENABLE) {
            ic->impl->shift_mode_enabled = true;
            caps_mode_check (ic, EINA_TRUE, EINA_TRUE);
        } else if (key.code == SHIFT_MODE_DISABLE) {
            ic->impl->shift_mode_enabled = false;
        }
    }

    if (key.code == SHIFT_MODE_ENABLE ||
        key.code == SHIFT_MODE_DISABLE) {
        process_key = EINA_FALSE;
    }

    if (filter_hotkeys (ic, _key) == false && process_key) {
        if (!_focused_ic || !_focused_ic->impl->is_on || !_focused_ic->impl->si ||
            !_focused_ic->impl->si->process_key_event (_key)) {
            if (_key.is_key_press ()) {
                char code = _key.get_ascii_code ();

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

                    feed_key_event (ic, _key, false);
                }
            }
        }
    }
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

//useless
#if 0
static void
panel_slot_request_help (int context)
{
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";

    if (ic && ic->impl) {
        panel_req_show_help (ic);
    }
}


static void
panel_slot_request_factory_menu (int context)
{
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";

    if (ic && ic->impl) {
        panel_req_show_factory_menu (ic);
    }
}
#endif

static void
panel_slot_change_factory (int context, const String &uuid)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " factory=" << uuid << " ic=" << ic << "\n";

    if (ic && ic->impl && ic->impl->si) {
        ic->impl->si->reset ();
        open_specific_factory (ic, uuid);
    }
}

static void
panel_slot_reset_keyboard_ise (int context)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";

    if (ic && ic->impl) {
        WideString wstr = ic->impl->preedit_string;
        if (ic->impl->need_commit_preedit) {
            _hide_preedit_string (ic->id, false);

            if (wstr.length ()) {
                wsc_context_commit_string (ic, utf8_wcstombs (wstr).c_str ());
                if (!check_valid_ic (ic))
                    return;
            }
        }

        if (ic->impl->si) {
            ic->impl->si->reset ();
        }
    }
}

static void
panel_slot_update_keyboard_ise (int context)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";
    _backend->add_module (_config, "socket", false);
}

static void
panel_slot_show_preedit_string (int context)
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
panel_slot_hide_preedit_string (int context)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    _hide_preedit_string (context, true);
}

static void
panel_slot_update_preedit_string (int context,
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

static void
panel_slot_get_surrounding_text (int context, int maxlen_before, int maxlen_after)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = find_ic (context);

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        int cursor = 0;
        WideString text = WideString ();
        slot_get_surrounding_text (ic->impl->si, text, cursor, maxlen_before, maxlen_after);
        String _str = utf8_wcstombs (text);
        g_info_manager->socket_update_surrounding_text (_str, (uint32)cursor);
    }
}

static void
panel_slot_delete_surrounding_text (int context, int offset, int len)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = find_ic (context);

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic)
        slot_delete_surrounding_text (ic->impl->si, offset, len);
}

static void
panel_slot_set_selection (int context, int start, int end)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = find_ic (context);

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic)
        slot_set_selection (ic->impl->si, start, end);
}

static void
panel_slot_send_private_command (int context, const String &command)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = find_ic (context);

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic)
        slot_send_private_command (ic->impl->si, command);
}

static void
panel_slot_update_displayed_candidate_number (int context, int number)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " number=" << number << " ic=" << ic << "\n";

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        ic->impl->si->update_displayed_candidate_number (number);
    }
}

static void
panel_slot_candidate_more_window_show (int context)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        ic->impl->si->candidate_more_window_show ();
    }
}

static void
panel_slot_candidate_more_window_hide (int context)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        ic->impl->si->candidate_more_window_hide ();
    }
}

static void
panel_slot_longpress_candidate (int context, int index)
{
    LOGD ("");
    WSCContextISF* ic = find_ic (context);
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " context=" << context << " index=" << index << " ic=" << ic << "\n";

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        ic->impl->si->longpress_candidate (index);
    }
}

static void
panel_slot_update_ise_input_context (int context, int type, int value)
{
    LOGD ("");
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
}

static void
panel_slot_update_isf_candidate_panel (int context, int type, int value)
{
    LOGD ("");
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
}

void
initialize (void)
{
    std::vector<String>     config_list;
    std::vector<String>     engine_list;
    std::vector<String>     helper_list;
    std::vector<String>     load_engine_list;

    std::vector<String>::iterator it;

    bool                    manual = false;
    bool                    socket = true;
    String                  config_module_name = "simple";

    LOGD ("Initializing Ecore ISF IMModule...\n");

    // Get system language.
    _language = scim_get_locale_language (scim_get_current_locale ());

    if (socket) {
        // If no Socket FrontEnd is running, then launch one.
        // And set manual to false.
        bool check_result = check_socket_frontend ();
        if (!check_result) {
            std::cerr << "Launching a ISF daemon with Socket FrontEnd...\n";
            //get modules list
            scim_get_imengine_module_list (engine_list);
            scim_get_helper_module_list (helper_list);

            for (it = engine_list.begin (); it != engine_list.end (); it++) {
                if (*it != "socket")
                    load_engine_list.push_back (*it);
            }
            for (it = helper_list.begin (); it != helper_list.end (); it++)
                load_engine_list.push_back (*it);
            const char *new_argv [] = { 0 };
            scim_launch (true,
                         config_module_name,
                         (load_engine_list.size () > 0 ? scim_combine_string_list (load_engine_list, ',') : "none"),
                         "socket",
                         (char **)new_argv);
            manual = false;
        }

        // If there is one Socket FrontEnd running and it's not manual mode,
        // then just use this Socket Frontend.
        if (!manual) {
            for (int i = 0; i < 200; ++i) {
                if (check_result) {
                    config_module_name = "socket";
                    load_engine_list.clear ();
                    load_engine_list.push_back ("socket");
                    break;
                }
                scim_usleep (50000);
                check_result = check_socket_frontend ();
            }
        }
    }

    // create backend
    _backend = new CommonBackEnd (_config, load_engine_list.size () > 0 ? load_engine_list : engine_list);

    if (_backend.null ()) {
        std::cerr << "Cannot create BackEnd Object!\n";
    } else {
        _backend->initialize (_config, load_engine_list.size () > 0 ? load_engine_list : engine_list, false, false);
        _fallback_factory = _backend->get_factory (SCIM_COMPOSE_KEY_FACTORY_UUID);
    }

    if (_fallback_factory.null ())
        _fallback_factory = new DummyIMEngineFactory ();

    _fallback_instance = _fallback_factory->create_instance (String ("UTF-8"), 0);
    _fallback_instance->signal_connect_commit_string (slot (fallback_commit_string_cb));

    if (!panel_initialize ()) {
        std::cerr << "Ecore IM Module: Cannot connect to Panel!\n";
    }
}

static void
finalize (void)
{
    LOGD ("Finalizing Ecore ISF IMModule...\n");

    // Reset this first so that the shared instance could be released correctly afterwards.
    _default_instance.reset ();

    SCIM_DEBUG_FRONTEND(2) << "Finalize all IC partially.\n";
    while (_used_ic_impl_list) {
        // In case in "shared input method" mode,
        // all contexts share only one instance,
        // so we need point the reference pointer correctly before finalizing.
        if (_used_ic_impl_list->si) {
            _used_ic_impl_list->si->set_frontend_data (static_cast <void*> (_used_ic_impl_list->parent));
        }
        if (_used_ic_impl_list->parent && _used_ic_impl_list->parent->ctx) {
            isf_wsc_context_del (_used_ic_impl_list->parent);
        }
    }

    delete_all_ic_impl ();

    _fallback_instance.reset ();
    _fallback_factory.reset ();

    SCIM_DEBUG_FRONTEND(2) << " Releasing BackEnd...\n";
    _backend.reset ();

    SCIM_DEBUG_FRONTEND(2) << " Releasing Config...\n";
    _config.reset ();
    _focused_ic = NULL;
    _ic_list = NULL;

    _scim_initialized = false;
    panel_finalize ();
}

static void
_display_input_language (WSCContextISF *ic)
{
}

static void
open_next_factory (WSCContextISF *ic)
{
    LOGD ("");

    if (!check_valid_ic (ic))
        return;

    if (!ic->impl->si)
        return;

    SCIM_DEBUG_FRONTEND(2) << __FUNCTION__ << " context=" << ic->id << "\n";
    IMEngineFactoryPointer sf = _backend->get_next_factory ("", "UTF-8", ic->impl->si->get_factory_uuid ());

    if (!sf.null ()) {
        turn_off_ic (ic);
        ic->impl->si = sf->create_instance ("UTF-8", ic->impl->si->get_id ());
        ic->impl->si->set_frontend_data (static_cast <void*> (ic));
        ic->impl->preedit_string = WideString ();
        ic->impl->preedit_caret = 0;
        attach_instance (ic->impl->si);
        _backend->set_default_factory (_language, sf->get_uuid ());
        g_info_manager->register_input_context (WAYLAND_MODULE_CLIENT_ID, ic->id, sf->get_uuid ());
        g_info_manager->socket_set_candidate_ui (ONE_LINE_CANDIDATE, FIXED_CANDIDATE_WINDOW);
        set_ic_capabilities (ic);
        turn_on_ic (ic);

        if (_shared_input_method) {
            _default_instance = ic->impl->si;
            ic->impl->shared_si = true;
        }
    }
}

static void
open_previous_factory (WSCContextISF *ic)
{
    LOGD ("");

    if (!check_valid_ic (ic))
        return;

    if (!ic->impl->si)
        return;

    SCIM_DEBUG_FRONTEND(2) << __FUNCTION__ << " context=" << ic->id << "\n";
    IMEngineFactoryPointer sf = _backend->get_previous_factory ("", "UTF-8", ic->impl->si->get_factory_uuid ());

    if (!sf.null ()) {
        turn_off_ic (ic);
        ic->impl->si = sf->create_instance ("UTF-8", ic->impl->si->get_id ());
        ic->impl->si->set_frontend_data (static_cast <void*> (ic));
        ic->impl->preedit_string = WideString ();
        ic->impl->preedit_caret = 0;
        attach_instance (ic->impl->si);
        _backend->set_default_factory (_language, sf->get_uuid ());
        g_info_manager->register_input_context (WAYLAND_MODULE_CLIENT_ID, ic->id, sf->get_uuid ());
        g_info_manager->socket_set_candidate_ui (ONE_LINE_CANDIDATE, FIXED_CANDIDATE_WINDOW);
        set_ic_capabilities (ic);
        turn_on_ic (ic);

        if (_shared_input_method) {
            _default_instance = ic->impl->si;
            ic->impl->shared_si = true;
        }
    }
}

static void
open_specific_factory (WSCContextISF *ic,
                       const String     &uuid)
{
    LOGD ("");

    if (!check_valid_ic (ic))
        return;

    if (!ic->impl->si)
        return;

    SCIM_DEBUG_FRONTEND(2) << __FUNCTION__ << " context=" << ic->id << "\n";

    // The same input method is selected, just turn on the IC.
    if (ic->impl->si->get_factory_uuid () == uuid) {
        turn_on_ic (ic);
        return;
    }

    IMEngineFactoryPointer sf = _backend->get_factory (uuid);

    if (uuid.length () && !sf.null ()) {
        turn_off_ic (ic);
        ic->impl->si = sf->create_instance ("UTF-8", ic->impl->si->get_id ());
        ic->impl->si->set_frontend_data (static_cast <void*> (ic));
        ic->impl->preedit_string = WideString ();
        ic->impl->preedit_caret = 0;
        attach_instance (ic->impl->si);
        _backend->set_default_factory (_language, sf->get_uuid ());
        g_info_manager->register_input_context (WAYLAND_MODULE_CLIENT_ID, ic->id, sf->get_uuid ());
        set_ic_capabilities (ic);
        turn_on_ic (ic);

        if (_shared_input_method) {
            _default_instance = ic->impl->si;
            ic->impl->shared_si = true;
        }
    } else {
        std::cerr << "open_specific_factory () is failed!!!!!!\n";
        LOGW ("open_specific_factory () is failed. uuid : %s\n", uuid.c_str ());

        // turn_off_ic comment out panel_req_update_factory_info ()
        //turn_off_ic (ic);
        if (ic->impl->is_on) {
            ic->impl->is_on = false;

            if (ic == _focused_ic) {
                ic->impl->si->focus_out ();

                panel_req_update_factory_info (ic);
                g_info_manager->socket_turn_off ();
            }

            //Record the IC on/off status
            if (_shared_input_method) {
                _config->write (String (SCIM_CONFIG_FRONTEND_IM_OPENED_BY_DEFAULT), false);
                _config->flush ();
            }

            if (ic->impl->use_preedit && ic->impl->preedit_string.length ()) {
                if (!check_valid_ic (ic))
                    return;

                if (check_valid_ic (ic))
                    ic->impl->preedit_started = false;
            }
        }
    }
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
attach_instance (const IMEngineInstancePointer &si)
{
    si->signal_connect_show_preedit_string (
        slot (slot_show_preedit_string));
    si->signal_connect_show_aux_string (
        slot (slot_show_aux_string));
    si->signal_connect_show_lookup_table (
        slot (slot_show_lookup_table));

    si->signal_connect_hide_preedit_string (
        slot (slot_hide_preedit_string));
    si->signal_connect_hide_aux_string (
        slot (slot_hide_aux_string));
    si->signal_connect_hide_lookup_table (
        slot (slot_hide_lookup_table));

    si->signal_connect_update_preedit_caret (
        slot (slot_update_preedit_caret));
    si->signal_connect_update_preedit_string (
        slot (slot_update_preedit_string));
    si->signal_connect_update_aux_string (
        slot (slot_update_aux_string));
    si->signal_connect_update_lookup_table (
        slot (slot_update_lookup_table));

    si->signal_connect_commit_string (
        slot (slot_commit_string));

    si->signal_connect_forward_key_event (
        slot (slot_forward_key_event));

    si->signal_connect_register_properties (
        slot (slot_register_properties));

    si->signal_connect_update_property (
        slot (slot_update_property));

    si->signal_connect_beep (
        slot (slot_beep));

    si->signal_connect_start_helper (
        slot (slot_start_helper));

    si->signal_connect_stop_helper (
        slot (slot_stop_helper));

    si->signal_connect_send_helper_event (
        slot (slot_send_helper_event));

    si->signal_connect_get_surrounding_text (
        slot (slot_get_surrounding_text));

    si->signal_connect_delete_surrounding_text (
        slot (slot_delete_surrounding_text));

    si->signal_connect_set_selection (
        slot (slot_set_selection));

    si->signal_connect_expand_candidate (
        slot (slot_expand_candidate));
    si->signal_connect_contract_candidate (
        slot (slot_contract_candidate));

    si->signal_connect_set_candidate_style (
        slot (slot_set_candidate_style));

    si->signal_connect_send_private_command (
        slot (slot_send_private_command));
}

// Implementation of slot functions
static void
slot_show_preedit_string (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic) {
        if (ic->impl->use_preedit) {
            if (!ic->impl->preedit_started) {
                if (check_valid_ic (ic))
                    ic->impl->preedit_started = true;
            }
        } else {
            g_info_manager->socket_show_preedit_string ();
        }
    }
}

static void
slot_show_aux_string (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        g_info_manager->socket_show_aux_string ();
}

static void
slot_show_lookup_table (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        g_info_manager->socket_show_lookup_table ();
}

static void
slot_hide_preedit_string (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic) {
        if (ic->impl->preedit_string.length ()) {
            ic->impl->preedit_string = WideString ();
            ic->impl->preedit_caret = 0;
            ic->impl->preedit_attrlist.clear ();
        }
        if (ic->impl->use_preedit) {
            if (check_valid_ic (ic) && ic->impl->preedit_started) {
                if (check_valid_ic (ic))
                    ic->impl->preedit_started = false;
            }
            wsc_context_send_preedit_string (ic);
        } else {
            g_info_manager->socket_hide_preedit_string ();
        }
    }
}

static void
slot_hide_aux_string (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        g_info_manager->socket_hide_aux_string ();
}

static void
slot_hide_lookup_table (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        g_info_manager->socket_hide_lookup_table ();
}

static void
slot_update_preedit_caret (IMEngineInstanceBase *si, int caret)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic && ic->impl->preedit_caret != caret) {
        ic->impl->preedit_caret = caret;
        if (ic->impl->use_preedit) {
            if (!ic->impl->preedit_started) {
                if (!check_valid_ic (ic))
                    return;

                ic->impl->preedit_started = true;
            }
        } else {
            g_info_manager->socket_update_preedit_caret (caret);
        }
    }
}

static void
slot_update_preedit_string (IMEngineInstanceBase *si,
                            const WideString & str,
                            const AttributeList & attrs,
                            int caret)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic && (ic->impl->preedit_string != str || str.length ())) {
        ic->impl->preedit_string   = str;
        ic->impl->preedit_attrlist = attrs;
        if (ic->impl->use_preedit) {
            if (!ic->impl->preedit_started) {
                if (!check_valid_ic (ic))
                    return;

                ic->impl->preedit_started = true;
            }
            if (caret >= 0 && caret <= (int)str.length ())
                ic->impl->preedit_caret = caret;
            else
                ic->impl->preedit_caret = str.length ();
            ic->impl->preedit_updating = true;
            if (check_valid_ic (ic))
                ic->impl->preedit_updating = false;
            wsc_context_send_preedit_string (ic);
        } else {
            String _str = utf8_wcstombs (str);
            g_info_manager->socket_update_preedit_string (_str, attrs, caret);
        }
    }
}

static void
slot_update_aux_string (IMEngineInstanceBase *si,
                        const WideString & str,
                        const AttributeList & attrs)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic) {
        String _str = utf8_wcstombs (str);
        g_info_manager->socket_update_aux_string (_str, attrs);
    }
}

static void
slot_commit_string (IMEngineInstanceBase *si,
                    const WideString & str)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic) {
        if (utf8_wcstombs (str) == String (" ") || utf8_wcstombs (str) == String ("　"))
            autoperiod_insert (ic);

        Eina_Bool auto_capitalized = EINA_FALSE;

        if (ic->impl) {
            if (wsc_context_input_panel_layout_get (ic) == ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL &&
                ic->impl->shift_mode_enabled &&
                ic->impl->autocapital_type != ECORE_IMF_AUTOCAPITAL_TYPE_NONE &&
                get_keyboard_mode () == TOOLBAR_HELPER_MODE) {
                char converted[2] = {'\0'};
                if (utf8_wcstombs (str).length () == 1) {
                    Eina_Bool uppercase;
                    switch (ic->impl->next_shift_status) {
                        case 0:
                            uppercase = caps_mode_check (ic, EINA_FALSE, EINA_FALSE);
                            break;
                        case SHIFT_MODE_OFF:
                            uppercase = EINA_FALSE;
                            ic->impl->next_shift_status = 0;
                            break;
                        case SHIFT_MODE_ON:
                            uppercase = EINA_TRUE;
                            ic->impl->next_shift_status = 0;
                            break;
                        case SHIFT_MODE_LOCK:
                            uppercase = EINA_TRUE;
                            break;
                        default:
                            uppercase = EINA_FALSE;
                    }
                    converted[0] = utf8_wcstombs (str).at (0);
                    if (uppercase) {
                        if (converted[0] >= 'a' && converted[0] <= 'z')
                            converted[0] -= 32;
                    } else {
                        if (converted[0] >= 'A' && converted[0] <= 'Z')
                            converted[0] += 32;
                    }

                    wsc_context_commit_string (ic, converted);
                    auto_capitalized = EINA_TRUE;
                }
            }
        }

        if (!auto_capitalized) {
            wsc_context_commit_string (ic, utf8_wcstombs (str).c_str ());
        }
    }
}

static void
slot_forward_key_event (IMEngineInstanceBase *si,
                        const KeyEvent & key)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && _focused_ic == ic) {
        if (!_fallback_instance->process_key_event (key)) {
            feed_key_event (ic, key, true);
        }
    }
}

static void
slot_update_lookup_table (IMEngineInstanceBase *si,
                          const LookupTable & table)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        g_info_manager->socket_update_lookup_table (table);
}

static void
slot_register_properties (IMEngineInstanceBase *si,
                          const PropertyList & properties)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        g_info_manager->socket_register_properties (properties);
}

static void
slot_update_property (IMEngineInstanceBase *si,
                      const Property & property)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        g_info_manager->socket_update_property (property);
}

static void
slot_beep (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";
}

static void
slot_start_helper (IMEngineInstanceBase *si,
                   const String &helper_uuid)
{
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());
    LOGD ("");
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " helper= " << helper_uuid << " context="
                            << (ic != NULL ? ic->id : -1) << " ic=" << ic
                            << " ic-uuid=" << ((ic && ic->impl && ic->impl->si) ? ic->impl->si->get_factory_uuid () : "") << "...\n";

    if (ic && ic->impl)
        g_info_manager->socket_start_helper (WAYLAND_MODULE_CLIENT_ID, ic->id, helper_uuid);
}

static void
slot_stop_helper (IMEngineInstanceBase *si,
                  const String &helper_uuid)
{
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());
    LOGD ("");
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " helper= " << helper_uuid << " context=" << (ic != NULL ? ic->id : -1) << " ic=" << ic << "...\n";

    if (ic && ic->impl)
        g_info_manager->socket_stop_helper (WAYLAND_MODULE_CLIENT_ID, ic->id, helper_uuid);
}

static void
slot_send_helper_event (IMEngineInstanceBase *si,
                        const String      &helper_uuid,
                        const Transaction &trans)
{
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());
    LOGD ("");
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << " helper= " << helper_uuid << " context="
                            << (ic != NULL ? ic->id : -1) << " ic=" << ic
                            << " ic-uuid=" << ((ic && ic->impl && ic->impl->si) ? ic->impl->si->get_factory_uuid () : "") << "...\n";

    if (ic && ic->impl)
        g_info_manager->socket_send_helper_event (WAYLAND_MODULE_CLIENT_ID, ic->id, helper_uuid, trans);
}

static bool
slot_get_surrounding_text (IMEngineInstanceBase *si,
                           WideString            &text,
                           int                   &cursor,
                           int                    maxlen_before,
                           int                    maxlen_after)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic) {
        char *surrounding = NULL;
        int   cursor_index;
        if (wsc_context_surrounding_get (_focused_ic, &surrounding, &cursor_index)) {
            SCIM_DEBUG_FRONTEND(2) << "Surrounding text: " << surrounding <<"\n";
            SCIM_DEBUG_FRONTEND(2) << "Cursor Index    : " << cursor_index <<"\n";

            if (!surrounding)
                return false;

            if (cursor_index < 0) {
                free (surrounding);
                surrounding = NULL;
                return false;
            }

            WideString before = utf8_mbstowcs (String (surrounding));
            free (surrounding);
            surrounding = NULL;

            if (cursor_index > (int)before.length ())
                return false;
            WideString after = before;
            before = before.substr (0, cursor_index);
            after =  after.substr (cursor_index, after.length () - cursor_index);
            if (maxlen_before > 0 && ((unsigned int)maxlen_before) < before.length ())
                before = WideString (before.begin () + (before.length () - maxlen_before), before.end ());
            else if (maxlen_before == 0)
                before = WideString ();
            if (maxlen_after > 0 && ((unsigned int)maxlen_after) < after.length ())
                after = WideString (after.begin (), after.begin () + maxlen_after);
            else if (maxlen_after == 0)
                after = WideString ();
            text = before + after;
            cursor = before.length ();
            return true;
        }
    }
    return false;
}

static bool
slot_delete_surrounding_text (IMEngineInstanceBase *si,
                              int                   offset,
                              int                   len)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && _focused_ic == ic) {
        wsc_context_delete_surrounding (_focused_ic, offset, len);
        return true;
    }
    return false;
}

static bool
slot_set_selection (IMEngineInstanceBase *si,
                    int              start,
                    int              end)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (_focused_ic && _focused_ic == ic) {
        wsc_context_set_selection (_focused_ic, start, end);
        return true;
    }
    return false;
}

static void
slot_expand_candidate (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        g_info_manager->expand_candidate ();
}

static void
slot_contract_candidate (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        g_info_manager->contract_candidate ();
}

static void
slot_set_candidate_style (IMEngineInstanceBase *si, ISF_CANDIDATE_PORTRAIT_LINE_T portrait_line, ISF_CANDIDATE_MODE_T mode)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        g_info_manager->socket_set_candidate_ui (portrait_line, mode);
}

static void
slot_send_private_command (IMEngineInstanceBase *si,
                           const String &command)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    WSCContextISF* ic = static_cast<WSCContextISF*> (si->get_frontend_data ());

    if (_focused_ic && _focused_ic == ic) {
        if (_focused_ic->im_ctx)
            wl_input_method_context_private_command (_focused_ic->im_ctx, _focused_ic->serial, command.c_str ());
    }
}

static void
reload_config_callback (const ConfigPointer &config)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");
    _frontend_hotkey_matcher.load_hotkeys (config);
    _imengine_hotkey_matcher.load_hotkeys (config);

    KeyEvent key;
    scim_string_to_key (key,
                        config->read (String (SCIM_CONFIG_HOTKEYS_FRONTEND_VALID_KEY_MASK),
                                      String ("Shift+Control+Alt+Lock")));

    _valid_key_mask = (key.mask > 0) ? (key.mask) : 0xFFFF;
    _valid_key_mask |= SCIM_KEY_ReleaseMask;
    // Special treatment for two backslash keys on jp106 keyboard.
    _valid_key_mask |= SCIM_KEY_QuirkKanaRoMask;

    _on_the_spot = config->read (String (SCIM_CONFIG_FRONTEND_ON_THE_SPOT), _on_the_spot);
    _shared_input_method = config->read (String (SCIM_CONFIG_FRONTEND_SHARED_INPUT_METHOD), _shared_input_method);
    _change_keyboard_mode_by_focus_move = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_CHANGE_KEYBOARD_MODE_BY_FOCUS_MOVE), _change_keyboard_mode_by_focus_move);
    _support_hw_keyboard_mode = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_SUPPORT_HW_KEYBOARD_MODE), _support_hw_keyboard_mode);

    // Get keyboard layout setting
    // Flush the global config first, in order to load the new configs from disk.
    scim_global_config_flush ();

    _keyboard_layout = scim_get_default_keyboard_layout ();
}

static void
fallback_commit_string_cb (IMEngineInstanceBase  *si,
                           const WideString      &str)
{
    SCIM_DEBUG_FRONTEND (1) << __FUNCTION__ << "...\n";
    LOGD ("");

    if (_focused_ic && _focused_ic->impl) {
        wsc_context_commit_string (_focused_ic, utf8_wcstombs (str).c_str ());
    }
}

class WaylandPanelAgent: public PanelAgentBase
{
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

        return true;
    }
    bool valid (void) const {
        return true;
    }

    void stop (void) {
        isf_wsc_context_shutdown ();
    }

public:
    void
    exit (int id, uint32 contextid) {
        LOGD ("client id:%d", id);
        panel_slot_exit (id);
    }

    void
    update_candidate_item_layout (int id, uint32 contextid, const std::vector<uint32>& row_items) {
        LOGD ("client id:%d", id);
        panel_slot_update_candidate_item_layout (contextid, row_items);
    }

    void
    update_lookup_table_page_size (int id, uint32 context_id, int page_size) {
        LOGD ("client id:%d", id);
        panel_slot_update_lookup_table_page_size (context_id, page_size);
    }

    void
    lookup_table_page_up (int id, uint32 context_id) {
        LOGD ("client id:%d", id);
        panel_slot_lookup_table_page_up (context_id);
    }

    void
    lookup_table_page_down (int id, uint32 context_id) {
        LOGD ("client id:%d", id);
        panel_slot_lookup_table_page_down (context_id);
    }

    void
    trigger_property (int id, uint32 context_id, const String& property) {
        LOGD ("client id:%d", id);
        panel_slot_trigger_property (context_id, property);
    }

    void
    process_helper_event (int id, uint32 context_id, const String target_uuid, const String helper_uuid, Transaction& trans) {
        LOGD ("client id:%d", id);
        panel_slot_process_helper_event (context_id, target_uuid, helper_uuid, trans);
    }

    //FIXME
    //useless
    //void
    //move_preedit_caret(int id, uint32 context_id, int caret_pos)
    //{
    //    LOGD("client id:%d", id);
    //    panel_slot_move_preedit_caret(context_id, caret_pos);
    //}

    void
    update_preedit_caret (int id, uint32 context_id, int caret) {
        LOGD ("client id:%d", id);
        panel_slot_update_preedit_caret (context_id, caret);
    }

    void
    select_aux (int id, uint32 context_id, int aux_index) {
        LOGD ("client id:%d", id);
        panel_slot_select_aux (context_id, aux_index);
    }

    void
    select_candidate (int id, uint32 context_id, uint32 cand_index) {
        LOGD ("client id:%d", id);
        panel_slot_select_candidate (context_id, cand_index);
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
#endif

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

    void
    show_preedit_string (int id, uint32 context_id) {
        LOGD ("client id:%d", id);
        panel_slot_show_preedit_string (context_id);
    }

    void
    hide_preedit_string (int id, uint32 context_id) {
        LOGD ("client id:%d", id);
        panel_slot_hide_preedit_string (context_id);
    }

    void
    update_preedit_string (int id, uint32 context_id,
                           const WideString&    str,
                           const AttributeList& attrs,
                           int caret) {
        LOGD ("client id:%d", id);
        panel_slot_update_preedit_string (context_id, str, attrs, caret);
    }

    void
    get_surrounding_text (int id, uint32 context_id, int maxlen_before, int maxlen_after) {
        LOGD ("client id:%d", id);
        panel_slot_get_surrounding_text (context_id, maxlen_before, maxlen_after);
    }

    void
    delete_surrounding_text (int id, uint32 context_id, int offset, int len) {
        LOGD ("client id:%d", id);
        panel_slot_delete_surrounding_text (context_id, offset, len);
    }

    void
    set_selection (int id, uint32 context_id, int start, int end) {
        LOGD ("client id:%d", id);
        panel_slot_set_selection (context_id, start, end);
    }

    void
    send_private_command (int id, uint32 context_id, const String& command) {
        LOGD ("client id:%d", id);
        panel_slot_send_private_command (context_id, command);
    }

    void
    update_displayed_candidate_number (int id, uint32 context_id, int number) {
        LOGD ("client id:%d", id);
        panel_slot_update_displayed_candidate_number (context_id, number);
    }

    void
    candidate_more_window_show (int id, uint32 context_id) {
        LOGD ("client id:%d", id);
        panel_slot_candidate_more_window_show (context_id);
    }

    void
    candidate_more_window_hide (int id, uint32 context_id) {
        LOGD ("client id:%d", id);
        panel_slot_candidate_more_window_hide (context_id);
    }

    void
    longpress_candidate (int id, uint32 context_id, int index) {
        LOGD ("client id:%d", id);
        panel_slot_longpress_candidate (context_id, index);
    }

    void
    update_ise_input_context (int id, uint32 context_id, uint32 type, uint32 value) {
        LOGD ("client id:%d", id);
        panel_slot_update_ise_input_context ((int)context_id, (int)type, (int)value);
    }

    void
    update_isf_candidate_panel (int id, uint32 context_id, int type, int value) {
        LOGD ("client id:%d", id);
        panel_slot_update_isf_candidate_panel (context_id, type, value);
    }

    void
    reload_config (int id)
    {
        LOGD ("client id:%d", id);
        reload_config_callback (_config);
    }

};

static scim::PanelAgentBase* instance = NULL;
extern "C" {

    EXAPI void scim_module_init (void)
    {
        LOGD ("");
    }

    EXAPI void scim_module_exit (void)
    {
        LOGD ("");
        if (instance) {
            delete instance;
            instance = NULL;
        }
    }

    EXAPI void scim_panel_agent_module_init (const scim::ConfigPointer& config)
    {
        LOGD ("");
        _config = config;
    }

    EXAPI scim::PanelAgentPointer scim_panel_agent_module_get_instance ()
    {
        if (!instance) {
            try {
                LOGD ("");
                instance = new WaylandPanelAgent ();
            } catch (...) {
                delete instance;
                instance = NULL;
            }
        }

        return scim::PanelAgentPointer (instance);
    }
}

/*
vi:ts=4:nowrap:expandtab
*/