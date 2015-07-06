/*
 * Copyright © 2013 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define Uses_SCIM_BACKEND
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_HELPER_MODULE
#define Uses_SCIM_HOTKEY
#define Uses_SCIM_PANEL_CLIENT

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/times.h>
#include <pthread.h>
#include <langinfo.h>
#include <unistd.h>

#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_Wayland.h>
#include <glib.h>
#include <vconf.h>
#include <vconf-keys.h>

#include "scim_private.h"
#include "scim.h"
#include "isf_wsc_context.h"
#include "isf_wsc_control_ui.h"
#include "input-method-client-protocol.h"
#include "text-client-protocol.h"

#define ENABLE_BACKKEY 1

using namespace scim;

struct _WSCContextISFImpl {
    WSCContextISF           *parent;
    IMEngineInstancePointer  si;
    Ecore_Wl_Window         *client_window;
    Evas                    *client_canvas;
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
                           client_canvas(NULL),
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

/* private functions */
static void     panel_slot_reload_config                (int                     context);
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
static void     panel_slot_move_preedit_caret           (int                     context,
                                                         int                     caret_pos);
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
static void     panel_slot_request_help                 (int                     context);
static void     panel_slot_request_factory_menu         (int                     context);
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

static void     panel_req_focus_in                      (WSCContextISF     *ic);
static void     panel_req_update_factory_info           (WSCContextISF     *ic);
static void     panel_req_update_spot_location          (WSCContextISF     *ic);
static void     panel_req_update_cursor_position        (WSCContextISF     *ic, int cursor_pos);
static void     panel_req_show_help                     (WSCContextISF     *ic);
static void     panel_req_show_factory_menu             (WSCContextISF     *ic);

/* Panel iochannel handler*/
static bool     panel_initialize                        (void);
static void     panel_finalize                          (void);
static Eina_Bool panel_iochannel_handler                (void                   *data,
                                                         Ecore_Fd_Handler       *fd_handler);

/* utility functions */
static bool     filter_hotkeys                          (WSCContextISF     *ic,
                                                         const KeyEvent         &key);
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

static void     slot_expand_candidate                   (IMEngineInstanceBase   *si);
static void     slot_contract_candidate                 (IMEngineInstanceBase   *si);

static void     slot_set_candidate_style                (IMEngineInstanceBase   *si,
                                                         ISF_CANDIDATE_PORTRAIT_LINE_T portrait_line,
                                                         ISF_CANDIDATE_MODE_T    mode);

static void     reload_config_callback                  (const ConfigPointer    &config);

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
static Connection                                       _config_connection;
static BackEndPointer                                   _backend;

static WSCContextISF                                   *_focused_ic                 = 0;

static bool                                             _scim_initialized           = false;

static int                                              _instance_count             = 0;
static int                                              _context_count              = 0;

static IMEngineFactoryPointer                           _fallback_factory;
static IMEngineInstancePointer                          _fallback_instance;
PanelClient                                             _panel_client;
static int                                              _panel_client_id            = 0;

static Ecore_Fd_Handler                                *_panel_iochannel_read_handler = 0;
static Ecore_Fd_Handler                                *_panel_iochannel_err_handler  = 0;

static bool                                             _on_the_spot                = true;
static bool                                             _shared_input_method        = false;
static double                                           space_key_time              = 0.0;

static Eina_Bool                                        autoperiod_allow            = EINA_FALSE;
static Eina_Bool                                        autocap_allow               = EINA_FALSE;
static Eina_Bool                                        desktop_mode                = EINA_FALSE;

static bool                                             _x_key_event_is_valid       = false;

typedef enum {
    INPUT_LANG_JAPANESE,
    INPUT_LANG_OTHER
} Input_Language;

static Input_Language                                   input_lang                  = INPUT_LANG_OTHER;

#define SHIFT_MODE_OFF  0xffe1
#define SHIFT_MODE_ON   0xffe2
#define SHIFT_MODE_LOCK 0xffe6
#define SHIFT_MODE_ENABLE 0x9fe7
#define SHIFT_MODE_DISABLE 0x9fe8

EAPI WSCContextISF *
get_focused_ic ()
{
    return _focused_ic;
}

EAPI int
get_panel_client_id (void)
{
    return _panel_client_id;
}

EAPI Eina_Bool
get_desktop_mode ()
{
    return desktop_mode;
}

EAPI void
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

    if (context_scim && context_scim->impl && context_scim == _focused_ic) {
        _panel_client.prepare (context_scim->id);
        context_scim->impl->si->set_prediction_allow (prediction);
        _panel_client.send ();
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

static void
autoperiod_insert (WSCContextISF *ctx)
{
    char *plain_str = NULL;
    char *markup_str = NULL;
    int cursor_pos = 0;
    Eina_Unicode *ustr = NULL;
    Eina_Unicode space_symbols[] = {' ', 0x00A0 /* no-break space */, 0x3000 /* ideographic space */};
    Eina_Unicode symbols[] = {' ', 0x00A0 /* no-break space */, 0x3000 /* ideographic space */,
                              ':', ';', '.', '!', '?', 0x00BF /* ¿ */, 0x00A1 /* ¡ */, 0x3002 /* 。 */};
    const int symbol_num = sizeof (symbols) / sizeof (symbols[0]);
    char *fullstop_mark = NULL;

    if (autoperiod_allow == EINA_FALSE)
        return;

    if (!ctx) return;

    Ecore_IMF_Input_Panel_Layout layout = wsc_context_input_panel_layout_get (ctx->ctx);
    if (layout != ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL)
        return;

    if ((ecore_time_get () - space_key_time) > DOUBLE_SPACE_INTERVAL)
        goto done;

    wsc_context_surrounding_get (ctx->ctx, &markup_str, &cursor_pos);
    if (!markup_str) goto done;

    // Convert into plain string
    plain_str = evas_textblock_text_markup_to_utf8 (NULL, markup_str);
    if (!plain_str) goto done;

    // Convert string from UTF-8 to unicode
    ustr = eina_unicode_utf8_to_unicode (plain_str, NULL);
    if (!ustr) goto done;

    if (cursor_pos < 2) goto done;

    if (check_symbol (ustr[cursor_pos-1], space_symbols, (sizeof (space_symbols) / sizeof (space_symbols[0]))) &&
        (!check_symbol (ustr[cursor_pos-2], symbols, symbol_num))) {
        wsc_context_delete_surrounding (ctx->ctx, -1, 1);

        if (input_lang == INPUT_LANG_JAPANESE) {
            fullstop_mark = strdup ("。");
        }
        else {
            fullstop_mark = strdup (".");
        }

        wsc_context_commit_string (ctx->ctx, fullstop_mark);

        if (fullstop_mark) {
            free (fullstop_mark);
        }
    }

done:
    if (markup_str) free (markup_str);
    if (plain_str) free (plain_str);
    if (ustr) free (ustr);
    space_key_time = ecore_time_get ();
}

static Eina_Bool
analyze_surrounding_text (WSCContextISF *ctx)
{
    char *plain_str = NULL;
    char *markup_str = NULL;
    Eina_Unicode puncs[] = {'\n','.', '!', '?', 0x00BF /* ¿ */, 0x00A1 /* ¡ */, 0x3002 /* 。 */};
    Eina_Unicode space_symbols[] = {' ', 0x00A0 /* no-break space */, 0x3000 /* ideographic space */};
    Eina_Unicode *ustr = NULL;
    Eina_Bool ret = EINA_FALSE;
    Eina_Bool detect_space = EINA_FALSE;
    int cursor_pos = 0;
    int i = 0;
    const int punc_num = sizeof (puncs) / sizeof (puncs[0]);
    WSCContextISF *context_scim;

    if (!ctx) return EINA_FALSE;
    context_scim = ctx;
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

    wsc_context_surrounding_get (ctx->ctx, &markup_str, &cursor_pos);
    if (!markup_str) goto done;

    if (cursor_pos == 0) {
        ret = EINA_TRUE;
        goto done;
    }

    // Convert into plain string
    plain_str = evas_textblock_text_markup_to_utf8 (NULL, markup_str);
    if (!plain_str) goto done;

    // Convert string from UTF-8 to unicode
    ustr = eina_unicode_utf8_to_unicode (plain_str, NULL);
    if (!ustr) goto done;

    if (eina_unicode_strlen (ustr) < (size_t)cursor_pos) goto done;

    if (cursor_pos >= 1) {
        if (context_scim->impl->autocapital_type == ECORE_IMF_AUTOCAPITAL_TYPE_WORD) {
            // Check space or no-break space
            if (check_symbol (ustr[cursor_pos-1], space_symbols, (sizeof (space_symbols) / sizeof (space_symbols[0])))) {
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
            if (check_symbol (ustr[i-1], space_symbols, (sizeof (space_symbols) / sizeof (space_symbols[0])))) {
                detect_space = EINA_TRUE;
                continue;
            }

            // Check punctuation and following the continuous space(s)
            if (detect_space && check_symbol (ustr[i-1], puncs, punc_num)) {
                ret = EINA_TRUE;
                goto done;
            }
            else {
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
    if (markup_str) free (markup_str);
    if (plain_str) free (plain_str);

    return ret;
}

EAPI Eina_Bool
caps_mode_check (WSCContextISF *ctx, Eina_Bool force, Eina_Bool noti)
{
    Eina_Bool uppercase;
    WSCContextISF *context_scim;

    if (hw_keyboard_num_get () > 0) return EINA_FALSE;

    if (!ctx) return EINA_FALSE;
    context_scim = ctx;

    if (!context_scim || !context_scim->impl)
        return EINA_FALSE;

    if (context_scim->impl->next_shift_status == SHIFT_MODE_LOCK) return EINA_TRUE;

    Ecore_IMF_Input_Panel_Layout layout = wsc_context_input_panel_layout_get (ctx->ctx);
    if (layout != ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL)
        return EINA_FALSE;

    // Check autocapital type
    if (wsc_context_input_panel_caps_lock_mode_get (ctx->ctx)) {
        uppercase = EINA_TRUE;
    } else {
        if (autocap_allow == EINA_FALSE)
            return EINA_FALSE;

        if (analyze_surrounding_text (ctx)) {
            uppercase = EINA_TRUE;
        } else {
            uppercase = EINA_FALSE;
        }
    }

    if (force) {
        context_scim->impl->next_shift_status = uppercase ? SHIFT_MODE_ON : SHIFT_MODE_OFF;
        if (noti)
            isf_wsc_context_input_panel_caps_mode_set (ctx, uppercase);
    } else {
        if (context_scim->impl->next_shift_status != (uppercase ? SHIFT_MODE_ON : SHIFT_MODE_OFF)) {
            context_scim->impl->next_shift_status = uppercase ? SHIFT_MODE_ON : SHIFT_MODE_OFF;
            if (noti)
                isf_wsc_context_input_panel_caps_mode_set (ctx, uppercase);
        }
    }

    return uppercase;
}

static void
get_input_language ()
{
    char *input_lang_str = vconf_get_str (VCONFKEY_ISF_INPUT_LANGUAGE);
    if (!input_lang_str) return;

    if (strcmp (input_lang_str, "ja_JP") == 0)
        input_lang = INPUT_LANG_JAPANESE;
    else
        input_lang = INPUT_LANG_OTHER;

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

EAPI void context_scim_imdata_get (WSCContextISF *ctx, void* data, int* length)
{
    WSCContextISF *context_scim = ctx;

    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (context_scim && context_scim->impl) {
        if (data && context_scim->impl->imdata)
            memcpy (data, context_scim->impl->imdata, context_scim->impl->imdata_size);

        if (length)
            *length = context_scim->impl->imdata_size;
    }
}

EAPI void
imengine_layout_set (WSCContextISF *ctx, Ecore_IMF_Input_Panel_Layout layout)
{
    WSCContextISF *context_scim = ctx;

    if (context_scim && context_scim->impl && context_scim == _focused_ic) {
        _panel_client.prepare (context_scim->id);
        context_scim->impl->si->set_layout (layout);
        _panel_client.send ();
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
EAPI WSCContextISF *
isf_wsc_context_new (void)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    int val;

    WSCContextISF *context_scim = new WSCContextISF;
    if (context_scim == NULL) {
        std::cerr << "memory allocation failed in " << __FUNCTION__ << "\n";
        return NULL;
    }

    if (_context_count == 0) {
        _context_count = getpid () % 50000;
    }
    context_scim->id = _context_count++;

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

    return context_scim;
}

EAPI void
isf_wsc_context_shutdown (void)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";
    ConfigBase::set (0);
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

EAPI void
isf_wsc_context_add (WSCContextISF *ctx)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *context_scim = ctx;

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
    context_scim->impl->client_canvas       = NULL;
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

    _panel_client.prepare (context_scim->id);
    _panel_client.register_input_context (context_scim->id, si->get_factory_uuid ());
    set_ic_capabilities (context_scim);
    _panel_client.send ();

    SCIM_DEBUG_FRONTEND(2) << "input context created: id = " << context_scim->id << "\n";
}

EAPI void
isf_wsc_context_del (WSCContextISF *ctx)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (!_ic_list) return;

    WSCContextISF *context_scim = ctx;

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
        _panel_client.prepare (context_scim->id);

        if (context_scim == _focused_ic)
            context_scim->impl->si->focus_out ();

        // Delete the instance.
        // FIXME:
        // In case the instance send out some helper event,
        // and this context has been focused out,
        // we need set the focused_ic to this context temporary.
        WSCContextISF *old_focused = _focused_ic;
        _focused_ic = context_scim;
        context_scim->impl->si.reset ();
        _focused_ic = old_focused;

        if (context_scim == _focused_ic) {
            _panel_client.turn_off (context_scim->id);
            _panel_client.focus_out (context_scim->id);
        }

        _panel_client.remove_input_context (context_scim->id);
        _panel_client.send ();

        if (context_scim->impl) {
            delete_ic_impl (context_scim->impl);
            context_scim->impl = 0;
        }
    }

    if (context_scim == _focused_ic)
        _focused_ic = 0;

    if (context_scim) {
        delete context_scim;
        context_scim = 0;
    }
}

EAPI void
isf_wsc_context_focus_in (WSCContextISF *ctx)
{
    WSCContextISF *context_scim = ctx;

    if (!context_scim)
        return;

    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__<< "(" << context_scim->id << ")...\n";

    if (_focused_ic) {
        if (_focused_ic == context_scim) {
            SCIM_DEBUG_FRONTEND(1) << "It's already focused.\n";
            return;
        }
        SCIM_DEBUG_FRONTEND(1) << "Focus out previous IC first: " << _focused_ic->id << "\n";
        if (_focused_ic->ctx)
            isf_wsc_context_focus_out (_focused_ic);
    }

    bool need_cap   = false;
    bool need_reset = false;
    bool need_reg   = false;

    if (context_scim && context_scim->impl) {
        _focused_ic = context_scim;

        _panel_client.send ();

        _panel_client.prepare (context_scim->id);

        // Handle the "Shared Input Method" mode.
        if (_shared_input_method) {
            SCIM_DEBUG_FRONTEND(2) << "shared input method.\n";
            IMEngineFactoryPointer factory = _backend->get_default_factory (_language, "UTF-8");
            if (!factory.null ()) {
                if (_default_instance.null () || _default_instance->get_factory_uuid () != factory->get_uuid ()) {
                    _default_instance = factory->create_instance ("UTF-8", _default_instance.null () ? _instance_count++ : _default_instance->get_id ());
                    attach_instance (_default_instance);
                    SCIM_DEBUG_FRONTEND(2) << "create new default instance: " << _default_instance->get_id () << " " << _default_instance->get_factory_uuid () << "\n";
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
            SCIM_DEBUG_FRONTEND(2) << "exit shared input method.\n";
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
                SCIM_DEBUG_FRONTEND(2) << "create new instance: " << context_scim->impl->si->get_id () << " " << context_scim->impl->si->get_factory_uuid () << "\n";
            }
        }

        context_scim->impl->si->set_frontend_data (static_cast <void*> (context_scim));

        if (need_reg) _panel_client.register_input_context (context_scim->id, context_scim->impl->si->get_factory_uuid ());
        if (need_cap) set_ic_capabilities (context_scim);

        panel_req_focus_in (context_scim);
//        panel_req_update_spot_location (context_scim);
//        panel_req_update_factory_info (context_scim);

        if (need_reset) context_scim->impl->si->reset ();
        if (context_scim->impl->is_on) {
            _panel_client.turn_on (context_scim->id);
//            _panel_client.hide_preedit_string (context_scim->id);
//            _panel_client.hide_aux_string (context_scim->id);
//            _panel_client.hide_lookup_table (context_scim->id);
            context_scim->impl->si->focus_in ();
            context_scim->impl->si->set_layout (wsc_context_input_panel_layout_get (ctx->ctx));
            context_scim->impl->si->set_prediction_allow (context_scim->impl->prediction_allow);
            if (context_scim->impl->imdata)
                context_scim->impl->si->set_imdata ((const char *)context_scim->impl->imdata, context_scim->impl->imdata_size);
        } else {
            _panel_client.turn_off (context_scim->id);
        }

        _panel_client.send ();
        if (caps_mode_check (ctx, EINA_FALSE, EINA_TRUE) == EINA_FALSE) {
            context_scim->impl->next_shift_status = 0;
        }
    }

    LOGD ("ctx : %p\n", ctx);
}

EAPI void
isf_wsc_context_focus_out (WSCContextISF *ctx)
{
    WSCContextISF *context_scim = ctx;

    if (!context_scim) return;

    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "(" << context_scim->id << ")...\n";

    if (context_scim && context_scim->impl && context_scim == _focused_ic) {

        LOGD ("ctx : %p\n", ctx);

        if (context_scim->impl->need_commit_preedit) {
            _hide_preedit_string (context_scim->id, false);

            wsc_context_commit_preedit_string (context_scim->ctx);
            _panel_client.prepare (context_scim->id);
            _panel_client.reset_input_context (context_scim->id);
            _panel_client.send ();
        }

        _panel_client.prepare (context_scim->id);

        context_scim->impl->si->focus_out ();
        context_scim->impl->si->reset ();
        context_scim->impl->cursor_pos = -1;

//          if (context_scim->impl->shared_si) context_scim->impl->si->reset ();

        _panel_client.focus_out (context_scim->id);
        _panel_client.send ();
        _focused_ic = 0;
    }
    _x_key_event_is_valid = false;
}

EAPI void
isf_wsc_context_reset (WSCContextISF *ctx)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *context_scim = ctx;

    if (context_scim && context_scim->impl && context_scim == _focused_ic) {
        _panel_client.prepare (context_scim->id);
        context_scim->impl->si->reset ();
        _panel_client.reset_input_context (context_scim->id);
        _panel_client.send ();

        if (context_scim->impl->need_commit_preedit) {
            _hide_preedit_string (context_scim->id, false);
            wsc_context_commit_preedit_string (context_scim->ctx);
        }
    }
}

EAPI void
isf_wsc_context_preedit_string_get (WSCContextISF *ctx, char** str, int *cursor_pos)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *context_scim = ctx;

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

EAPI void
isf_wsc_context_prediction_allow_set (WSCContextISF* ctx, Eina_Bool prediction)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " = " << (prediction == EINA_TRUE ? "true" : "false") << "...\n";

    WSCContextISF *context_scim = ctx;

    if (context_scim && context_scim->impl && context_scim->impl->prediction_allow != prediction) {
        context_scim->impl->prediction_allow = prediction;
        set_prediction_allow (ctx, prediction);
    }
}

EAPI Eina_Bool
isf_wsc_context_prediction_allow_get (WSCContextISF* ctx)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *context_scim = ctx;

    Eina_Bool ret = EINA_FALSE;
    if (context_scim && context_scim->impl) {
        ret = context_scim->impl->prediction_allow;
    } else {
        std::cerr << __FUNCTION__ << " failed!!!\n";
    }
    return ret;
}

EAPI void
isf_wsc_context_autocapital_type_set (WSCContextISF* ctx, Ecore_IMF_Autocapital_Type autocapital_type)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " = " << autocapital_type << "...\n";

    WSCContextISF *context_scim = ctx;

    if (context_scim && context_scim->impl && context_scim->impl->autocapital_type != autocapital_type) {
        context_scim->impl->autocapital_type = autocapital_type;

        if (context_scim->impl->si && context_scim == _focused_ic) {
            LOGD ("ctx : %p. set autocapital type : %d\n", ctx, autocapital_type);
            _panel_client.prepare (context_scim->id);
            context_scim->impl->si->set_autocapital_type (autocapital_type);
            _panel_client.send ();
        }
    }
}

static void
wsc_commit_preedit (weescim *ctx)
{
    char *surrounding_text;

    if (!ctx || !ctx->preedit_str ||
        strlen (ctx->preedit_str) == 0)
        return;

    wl_input_method_context_cursor_position (ctx->im_ctx,
                                             0, 0);

    wl_input_method_context_commit_string (ctx->im_ctx,
                                           ctx->serial,
                                           ctx->preedit_str);

    if (ctx->surrounding_text) {
        surrounding_text = insert_text (ctx->surrounding_text,
                                        ctx->surrounding_cursor,
                                        ctx->preedit_str);

        free (ctx->surrounding_text);
        ctx->surrounding_text = surrounding_text;
        ctx->surrounding_cursor += strlen (ctx->preedit_str);
    } else {
        ctx->surrounding_text = strdup (ctx->preedit_str);
        ctx->surrounding_cursor = strlen (ctx->preedit_str);
    }

    if (ctx->preedit_str)
        free (ctx->preedit_str);

    ctx->preedit_str = strdup ("");
}

static void
wsc_send_preedit (weescim *ctx, int32_t cursor)
{
    if (!ctx) return;

    uint32_t index = strlen (ctx->preedit_str);

    if (ctx->preedit_style)
        wl_input_method_context_preedit_styling (ctx->im_ctx,
                                                 0,
                                                 strlen (ctx->preedit_str),
                                                 ctx->preedit_style);
    if (cursor > 0)
        index = cursor;

    wl_input_method_context_preedit_cursor (ctx->im_ctx, index);
    wl_input_method_context_preedit_string (ctx->im_ctx,
                                            ctx->serial,
                                            ctx->preedit_str,
                                            ctx->preedit_str);
}

bool wsc_context_surrounding_get (weescim *ctx, char **text, int *cursor_pos)
{
    if (!ctx)
        return false;

    if (text) {
        if (ctx->surrounding_text)
            *text = strdup (ctx->surrounding_text);
        else
            *text = strdup ("");
    }

    if (cursor_pos)
        *cursor_pos = ctx->surrounding_cursor;

    return true;
}

Ecore_IMF_Input_Panel_Layout wsc_context_input_panel_layout_get (weescim *ctx)
{
    Ecore_IMF_Input_Panel_Layout layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;

    if (!ctx)
        return layout;

    switch (ctx->content_purpose) {
        case WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS:
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
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_TERMINAL:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_TERMINAL;
            break;
        default:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;
            break;
    }

    return layout;
}

Ecore_IMF_Autocapital_Type wsc_context_autocapital_type_get (weescim *ctx)
{
    Ecore_IMF_Autocapital_Type autocapital_type = ECORE_IMF_AUTOCAPITAL_TYPE_NONE;

    if (!ctx)
        return autocapital_type;

    if (ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_AUTO_CAPITALIZATION)
        autocapital_type = ECORE_IMF_AUTOCAPITAL_TYPE_SENTENCE;
    else if (ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_UPPERCASE)
        autocapital_type = ECORE_IMF_AUTOCAPITAL_TYPE_ALLCHARACTER;
    else
        autocapital_type = ECORE_IMF_AUTOCAPITAL_TYPE_NONE;

    return autocapital_type;
}

bool wsc_context_input_panel_caps_lock_mode_get (weescim *ctx)
{
    if (!ctx)
        return false;

    if (ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_UPPERCASE)
        return true;

    return false;
}

Ecore_IMF_Input_Panel_Lang wsc_context_input_panel_language_get (weescim *ctx)
{
    Ecore_IMF_Input_Panel_Lang language = ECORE_IMF_INPUT_PANEL_LANG_AUTOMATIC;

    if (!ctx)
        return language;

    if (ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_LATIN)
        language = ECORE_IMF_INPUT_PANEL_LANG_ALPHABET;
    else
        language = ECORE_IMF_INPUT_PANEL_LANG_AUTOMATIC;

    return language;
}

bool wsc_context_input_panel_password_mode_get (weescim *ctx)
{
    if (ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_PASSWORD)
        return true;

    if (wsc_context_input_panel_layout_get (ctx) == ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD)
        return true;

    return false;
}

Ecore_IMF_Input_Hints wsc_context_input_hint_get (weescim *ctx)
{
    int input_hint = ECORE_IMF_INPUT_HINT_NONE;

    if (!ctx)
        return (Ecore_IMF_Input_Hints)input_hint;

    if (ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_SENSITIVE_DATA)
        input_hint |= ECORE_IMF_INPUT_HINT_SENSITIVE_DATA;
    else
        input_hint &= ~ECORE_IMF_INPUT_HINT_SENSITIVE_DATA;

    if (ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_AUTO_COMPLETION)
        input_hint |= ECORE_IMF_INPUT_HINT_AUTO_COMPLETE;
    else
        input_hint &= ~ECORE_IMF_INPUT_HINT_AUTO_COMPLETE;

    return (Ecore_IMF_Input_Hints)input_hint;
}

void wsc_context_delete_surrounding (weescim *ctx, int offset, int len)
{
    if (!ctx)
        return;

    wl_input_method_context_delete_surrounding_text (ctx->im_ctx, offset, len);
}

void wsc_context_commit_string (weescim *ctx, const char *str)
{
    if (!ctx)
        return;

    if (ctx->preedit_str) {
        free (ctx->preedit_str);
        ctx->preedit_str = NULL;
    }

    ctx->preedit_str = strdup (str);
    wsc_commit_preedit (ctx);
}

void wsc_context_commit_preedit_string (weescim *ctx)
{
    char *preedit_str = NULL;
    int cursor_pos = 0;

    if (!ctx)
        return;

    if (ctx->wsc_ctx)
        isf_wsc_context_preedit_string_get (ctx->wsc_ctx, &preedit_str, &cursor_pos);

    if (ctx->preedit_str) {
        free (ctx->preedit_str);
        ctx->preedit_str = NULL;
    }

    ctx->preedit_str = preedit_str;
    wsc_commit_preedit (ctx);
}

void wsc_context_send_preedit_string (weescim *ctx)
{
    char *preedit_str = NULL;
    int cursor_pos = 0;

    if (!ctx)
        return;

    if (ctx->wsc_ctx)
        isf_wsc_context_preedit_string_get (ctx->wsc_ctx, &preedit_str, &cursor_pos);

    if (ctx->preedit_str) {
        free (ctx->preedit_str);
        ctx->preedit_str = NULL;
    }

    ctx->preedit_str = preedit_str;
    wsc_send_preedit (ctx, cursor_pos);
}

void wsc_context_send_key (weescim *ctx, uint32_t keysym, uint32_t modifiers, uint32_t time, bool press)
{
    if (!ctx || !ctx->im_ctx)
        return;

    ctx->modifiers = modifiers;

    if (press) {
        wl_input_method_context_keysym (ctx->im_ctx, ctx->serial, time,
                                        keysym, WL_KEYBOARD_KEY_STATE_PRESSED, ctx->modifiers);
    }
    else {
        wl_input_method_context_keysym (ctx->im_ctx, ctx->serial, time,
                                        keysym, WL_KEYBOARD_KEY_STATE_RELEASED, ctx->modifiers);
    }
}

static void
turn_on_ic (WSCContextISF *ic)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic && ic->impl && !ic->impl->is_on) {
        ic->impl->is_on = true;

        if (ic == _focused_ic) {
            panel_req_focus_in (ic);
//            panel_req_update_spot_location (ic);
            panel_req_update_factory_info (ic);
            _panel_client.turn_on (ic->id);
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
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic && ic->impl && ic->impl->is_on) {
        ic->impl->is_on = false;

        if (ic == _focused_ic) {
            ic->impl->si->focus_out ();

//            panel_req_update_factory_info (ic);
            _panel_client.turn_off (ic->id);
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
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic && ic->impl) {
        unsigned int cap = SCIM_CLIENT_CAP_ALL_CAPABILITIES;

        if (!_on_the_spot || !ic->impl->use_preedit)
            cap -= SCIM_CLIENT_CAP_ONTHESPOT_PREEDIT;

        ic->impl->si->update_client_capabilities (cap);
    }
}

static bool
check_socket_frontend (void)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

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

/* Panel Requestion functions. */
static void
panel_req_show_help (WSCContextISF *ic)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    String help;

    help =  String (_("Smart Common Input Method platform ")) +
            String (SCIM_VERSION) +
            String (_("\n(C) 2002-2005 James Su <suzhe@tsinghua.org.cn>\n\n"));

    if (ic && ic->impl) {
        IMEngineFactoryPointer sf = _backend->get_factory (ic->impl->si->get_factory_uuid ());
        if (sf) {
            help += utf8_wcstombs (sf->get_name ());
            help += String (_(":\n\n"));

            help += utf8_wcstombs (sf->get_help ());
            help += String (_("\n\n"));

            help += utf8_wcstombs (sf->get_credits ());
        }
        _panel_client.show_help (ic->id, help);
    }
}

static void
panel_req_show_factory_menu (WSCContextISF *ic)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

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
        _panel_client.show_factory_menu (ic->id, menu);
}

static void
panel_req_update_factory_info (WSCContextISF *ic)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic && ic->impl && ic == _focused_ic) {
        PanelFactoryInfo info;
        if (ic->impl->is_on) {
            IMEngineFactoryPointer sf = _backend->get_factory (ic->impl->si->get_factory_uuid ());
            if (sf)
                info = PanelFactoryInfo (sf->get_uuid (), utf8_wcstombs (sf->get_name ()), sf->get_language (), sf->get_icon_file ());
        } else {
            info = PanelFactoryInfo (String (""), String (_("English Keyboard")), String ("C"), String (SCIM_KEYBOARD_ICON_FILE));
        }
        _panel_client.update_factory_info (ic->id, info);
    }
}

static void
panel_req_focus_in (WSCContextISF *ic)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic && ic->impl && ic->impl->si)
        _panel_client.focus_in (ic->id, ic->impl->si->get_factory_uuid ());
}

static void
panel_req_update_spot_location (WSCContextISF *ic)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic && ic->impl)
        _panel_client.update_spot_location (ic->id, ic->impl->cursor_x, ic->impl->cursor_y, ic->impl->cursor_top_y);
}

static void
panel_req_update_cursor_position (WSCContextISF *ic, int cursor_pos)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic)
        _panel_client.update_cursor_position (ic->id, cursor_pos);
}

static bool
filter_hotkeys (WSCContextISF *ic, const KeyEvent &key)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

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
                _panel_client.hide_aux_string (ic->id);
                _panel_client.hide_lookup_table (ic->id);
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
            _panel_client.start_helper (ic->id, info.uuid);
        ret = true;
    }
    return ret;
}

static bool
panel_initialize (void)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    String display_name;
    {
        const char *p = getenv ("DISPLAY");
        if (p) display_name = String (p);
    }

    if (_panel_client.open_connection (_config->get_name (), display_name) >= 0) {
        if (_panel_client.get_client_id (_panel_client_id)) {
            _panel_client.prepare (0);
            _panel_client.register_client (_panel_client_id);
            _panel_client.send ();
        }

        int fd = _panel_client.get_connection_number ();

        _panel_iochannel_read_handler = ecore_main_fd_handler_add (fd, ECORE_FD_READ, panel_iochannel_handler, NULL, NULL, NULL);
//        _panel_iochannel_err_handler  = ecore_main_fd_handler_add (fd, ECORE_FD_ERROR, panel_iochannel_handler, NULL, NULL, NULL);

        SCIM_DEBUG_FRONTEND(2) << " Panel FD= " << fd << "\n";

        WSCContextISF *context_scim = _ic_list;
        while (context_scim != NULL) {
            if (context_scim->impl && context_scim->impl->si) {
                _panel_client.prepare (context_scim->id);
                _panel_client.register_input_context (context_scim->id, context_scim->impl->si->get_factory_uuid ());
                _panel_client.send ();
            }
            context_scim = context_scim->next;
        }

        if (_focused_ic) {
            _focused_ic = 0;
        }

        return true;
    }
    std::cerr << "panel_initialize () failed!!!\n";
    return false;
}

static void
panel_finalize (void)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    _panel_client.close_connection ();

    if (_panel_iochannel_read_handler) {
        ecore_main_fd_handler_del (_panel_iochannel_read_handler);
        _panel_iochannel_read_handler = 0;
    }
    if (_panel_iochannel_err_handler) {
        ecore_main_fd_handler_del (_panel_iochannel_err_handler);
        _panel_iochannel_err_handler = 0;
    }
}

static Eina_Bool
panel_iochannel_handler (void *data, Ecore_Fd_Handler *fd_handler)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (fd_handler == _panel_iochannel_read_handler) {
        if (_panel_client.has_pending_event () && !_panel_client.filter_event ()) {
            panel_finalize ();
            panel_initialize ();
            return ECORE_CALLBACK_CANCEL;
        }
    } else if (fd_handler == _panel_iochannel_err_handler) {
        panel_finalize ();
        panel_initialize ();
        return ECORE_CALLBACK_CANCEL;
    }
    return ECORE_CALLBACK_RENEW;
}

/* Panel Slot functions */
static void
panel_slot_reload_config (int context)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";
}

static void
panel_slot_exit (int /* context */)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    finalize ();
}

static void
panel_slot_update_candidate_item_layout (int context, const std::vector<uint32> &row_items)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " row size=" << row_items.size () << " ic=" << ic << "\n";
    if (ic && ic->impl) {
        _panel_client.prepare (ic->id);
        ic->impl->si->update_candidate_item_layout (row_items);
        _panel_client.send ();
    }
}

static void
panel_slot_update_lookup_table_page_size (int context, int page_size)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " page_size=" << page_size << " ic=" << ic << "\n";
    if (ic && ic->impl) {
        _panel_client.prepare (ic->id);
        ic->impl->si->update_lookup_table_page_size (page_size);
        _panel_client.send ();
    }
}

static void
panel_slot_lookup_table_page_up (int context)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";
    if (ic && ic->impl) {
        _panel_client.prepare (ic->id);
        ic->impl->si->lookup_table_page_up ();
        _panel_client.send ();
    }
}

static void
panel_slot_lookup_table_page_down (int context)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";
    if (ic && ic->impl) {
        _panel_client.prepare (ic->id);
        ic->impl->si->lookup_table_page_down ();
        _panel_client.send ();
    }
}

static void
panel_slot_trigger_property (int context, const String &property)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " property=" << property << " ic=" << ic << "\n";
    if (ic && ic->impl) {
        _panel_client.prepare (ic->id);
        ic->impl->si->trigger_property (property);
        _panel_client.send ();
    }
}

static void
panel_slot_process_helper_event (int context, const String &target_uuid, const String &helper_uuid, const Transaction &trans)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " target=" << target_uuid
                           << " helper=" << helper_uuid << " ic=" << ic << " ic->impl=" << (ic != NULL ? ic->impl : 0) << " ic-uuid="
                           << ((ic && ic->impl) ? ic->impl->si->get_factory_uuid () : "" ) << "\n";
    if (ic && ic->impl && ic->impl->si->get_factory_uuid () == target_uuid) {
        _panel_client.prepare (ic->id);
        SCIM_DEBUG_FRONTEND(2) << "call process_helper_event\n";
        ic->impl->si->process_helper_event (helper_uuid, trans);
        _panel_client.send ();
    }
}

static void
panel_slot_move_preedit_caret (int context, int caret_pos)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " caret=" << caret_pos << " ic=" << ic << "\n";
    if (ic && ic->impl) {
        _panel_client.prepare (ic->id);
        ic->impl->si->move_preedit_caret (caret_pos);
        _panel_client.send ();
    }
}

static void
panel_slot_update_preedit_caret (int context, int caret)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " caret=" << caret << " ic=" << ic << "\n";

    if (ic && ic->impl && _focused_ic == ic && ic->impl->preedit_caret != caret) {
        ic->impl->preedit_caret = caret;
        if (ic->impl->use_preedit) {
            if (!ic->impl->preedit_started) {
                if (check_valid_ic (ic))
                    ic->impl->preedit_started = true;
            }
        } else {
            _panel_client.prepare (ic->id);
            _panel_client.update_preedit_caret (ic->id, caret);
            _panel_client.send ();
        }
    }
}

static void
panel_slot_select_aux (int context, int aux_index)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " aux=" << aux_index << " ic=" << ic << "\n";
    if (ic && ic->impl) {
        _panel_client.prepare (ic->id);
        ic->impl->si->select_aux (aux_index);
        _panel_client.send ();
    }
}

static void
panel_slot_select_candidate (int context, int cand_index)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " candidate=" << cand_index << " ic=" << ic << "\n";
    if (ic && ic->impl) {
        _panel_client.prepare (ic->id);
        ic->impl->si->select_candidate (cand_index);
        _panel_client.send ();
    }
}

static Eina_Bool
feed_key_event (WSCContextISF *ic, const KeyEvent &key, bool fake)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

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
    WSCContextISF *ic = find_ic (context);
    Eina_Bool process_key = EINA_TRUE;
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " key=" << key.get_key_string () << " ic=" << ic << "\n";

    if (!(ic && ic->impl))
        return;

    if ((_focused_ic != NULL) && (_focused_ic != ic))
        return;

    KeyEvent _key = key;
    if (key.is_key_press () &&
        wsc_context_input_panel_layout_get (ic->ctx) == ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL) {
        if (key.code == SHIFT_MODE_OFF ||
            key.code == SHIFT_MODE_ON ||
            key.code == SHIFT_MODE_LOCK) {
            ic->impl->next_shift_status = _key.code;
        } else if (key.code == SHIFT_MODE_ENABLE ) {
            ic->impl->shift_mode_enabled = true;
            caps_mode_check (ic, EINA_TRUE, EINA_TRUE);
        } else if (key.code == SHIFT_MODE_DISABLE ) {
            ic->impl->shift_mode_enabled = false;
        }
    }

    if (key.code != SHIFT_MODE_OFF &&
        key.code != SHIFT_MODE_ON &&
        key.code != SHIFT_MODE_LOCK &&
        key.code != SHIFT_MODE_ENABLE &&
        key.code != SHIFT_MODE_DISABLE) {
        if (ic->impl->preedit_string.length () < 1) {
            if (!((key.code >= 'a' && key.code <= 'z') ||
                (key.code >= 'A' && key.code <= 'Z'))) {
                if (feed_key_event (ic, _key, false)) return;
            }
        }
    }

    if (key.code == SHIFT_MODE_ENABLE ||
        key.code == SHIFT_MODE_DISABLE) {
        process_key = EINA_FALSE;
    }

    _panel_client.prepare (ic->id);

    if (!filter_hotkeys (ic, _key)) {
        if (process_key) {
            if (!_focused_ic || !_focused_ic->impl->is_on ||
                    !_focused_ic->impl->si->process_key_event (_key)) {
                _fallback_instance->process_key_event (_key);
            }
        }
    }

    _panel_client.send ();
}

static void
panel_slot_commit_string (int context, const WideString &wstr)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " str=" << utf8_wcstombs (wstr) << " ic=" << ic << "\n";

    if (ic && ic->impl) {
        if (_focused_ic != ic)
            return;

        if (utf8_wcstombs (wstr) == String (" ") || utf8_wcstombs (wstr) == String ("　"))
            autoperiod_insert (ic);

        if (ic->impl->need_commit_preedit)
            _hide_preedit_string (ic->id, false);
        wsc_context_commit_string (ic->ctx, utf8_wcstombs (wstr).c_str ());
    }
}

static void
panel_slot_forward_key_event (int context, const KeyEvent &key)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " key=" << key.get_key_string () << " ic=" << ic << "\n";

    if (!(ic && ic->impl))
        return;

    if ((_focused_ic != NULL) && (_focused_ic != ic))
        return;

    if (strlen (key.get_key_string ().c_str ()) >= 116)
        return;

    feed_key_event (ic, key, true);
}

static void
panel_slot_request_help (int context)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";
    if (ic && ic->impl) {
        _panel_client.prepare (ic->id);
        panel_req_show_help (ic);
        _panel_client.send ();
    }
}

static void
panel_slot_request_factory_menu (int context)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";
    if (ic && ic->impl) {
        _panel_client.prepare (ic->id);
        panel_req_show_factory_menu (ic);
        _panel_client.send ();
    }
}

static void
panel_slot_change_factory (int context, const String &uuid)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " factory=" << uuid << " ic=" << ic << "\n";
    if (ic && ic->impl) {
        _panel_client.prepare (ic->id);
        ic->impl->si->reset ();
        open_specific_factory (ic, uuid);
        _panel_client.send ();
    }
}

static void
panel_slot_reset_keyboard_ise (int context)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";
    if (ic && ic->impl) {
        WideString wstr = ic->impl->preedit_string;
        if (ic->impl->need_commit_preedit) {
            _hide_preedit_string (ic->id, false);

            if (wstr.length ()) {
                wsc_context_commit_string (ic->ctx, utf8_wcstombs (wstr).c_str ());
                if (!check_valid_ic (ic))
                    return;
            }
        }
        _panel_client.prepare (ic->id);
        ic->impl->si->reset ();
        _panel_client.send ();
    }
}

static void
panel_slot_update_keyboard_ise (int context)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";

    _backend->add_module (_config, "socket", false);
}

static void
panel_slot_show_preedit_string (int context)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << "\n";

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
            _panel_client.prepare (ic->id);
            _panel_client.show_preedit_string (ic->id);
            _panel_client.send ();
        }
    }
}

static void
_hide_preedit_string (int context, bool update_preedit)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = find_ic (context);

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
            wsc_context_send_preedit_string (ic->ctx);
        } else {
            _panel_client.prepare (ic->id);
            _panel_client.hide_preedit_string (ic->id);
            _panel_client.send ();
        }
    }
}

static void
panel_slot_hide_preedit_string (int context)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    _hide_preedit_string (context, true);
}

static void
panel_slot_update_preedit_string (int context,
                                  const WideString    &str,
                                  const AttributeList &attrs,
                                  int caret)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = find_ic (context);

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
                wsc_context_send_preedit_string (ic->ctx);
            } else {
                _panel_client.prepare (ic->id);
                _panel_client.update_preedit_string (ic->id, str, attrs, caret);
                _panel_client.send ();
            }
        }
    }
}

static void
panel_slot_get_surrounding_text (int context, int maxlen_before, int maxlen_after)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = find_ic (context);

    if (ic && ic->impl && _focused_ic == ic && ic->impl->si) {
        int cursor = 0;
        WideString text = WideString ();
        slot_get_surrounding_text (ic->impl->si, text, cursor, maxlen_before, maxlen_after);
        _panel_client.prepare (ic->id);
        _panel_client.update_surrounding_text (ic->id, text, cursor);
        _panel_client.send ();
    }
}

static void
panel_slot_delete_surrounding_text (int context, int offset, int len)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = find_ic (context);

    if (ic && ic->impl && _focused_ic == ic && ic->impl->si)
        slot_delete_surrounding_text (ic->impl->si, offset, len);
}

static void
panel_slot_update_displayed_candidate_number (int context, int number)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " number=" << number << " ic=" << ic << "\n";
    if (ic && ic->impl && _focused_ic == ic && ic->impl->si) {
        _panel_client.prepare (ic->id);
        ic->impl->si->update_displayed_candidate_number (number);
        _panel_client.send ();
    }
}

static void
panel_slot_candidate_more_window_show (int context)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";
    if (ic && ic->impl && _focused_ic == ic && ic->impl->si) {
        _panel_client.prepare (ic->id);
        ic->impl->si->candidate_more_window_show ();
        _panel_client.send ();
    }
}

static void
panel_slot_candidate_more_window_hide (int context)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";
    if (ic && ic->impl && _focused_ic == ic && ic->impl->si) {
        _panel_client.prepare (ic->id);
        ic->impl->si->candidate_more_window_hide ();
        _panel_client.send ();
    }
}

static void
panel_slot_longpress_candidate (int context, int index)
{
    WSCContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " index=" << index << " ic=" << ic << "\n";
    if (ic && ic->impl && _focused_ic == ic && ic->impl->si) {
        _panel_client.prepare (ic->id);
        ic->impl->si->longpress_candidate (index);
        _panel_client.send ();
    }
}

static void
panel_slot_update_ise_input_context (int context, int type, int value)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";
}

static void
panel_slot_update_isf_candidate_panel (int context, int type, int value)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";
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

    if (config_module_name != "dummy") {
        _config = ConfigBase::get (true, config_module_name);
    }

    if (_config.null ()) {
        LOGD ("Config module cannot be loaded, using dummy Config.\n");
        _config = new DummyConfig ();
        config_module_name = "dummy";
    }

    reload_config_callback (_config);
    _config_connection = _config->signal_connect_reload (slot (reload_config_callback));

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

    // Attach Panel Client signal.
    _panel_client.signal_connect_reload_config                 (slot (panel_slot_reload_config));
    _panel_client.signal_connect_exit                          (slot (panel_slot_exit));
    _panel_client.signal_connect_update_candidate_item_layout  (slot (panel_slot_update_candidate_item_layout));
    _panel_client.signal_connect_update_lookup_table_page_size (slot (panel_slot_update_lookup_table_page_size));
    _panel_client.signal_connect_lookup_table_page_up          (slot (panel_slot_lookup_table_page_up));
    _panel_client.signal_connect_lookup_table_page_down        (slot (panel_slot_lookup_table_page_down));
    _panel_client.signal_connect_trigger_property              (slot (panel_slot_trigger_property));
    _panel_client.signal_connect_process_helper_event          (slot (panel_slot_process_helper_event));
    _panel_client.signal_connect_move_preedit_caret            (slot (panel_slot_move_preedit_caret));
    _panel_client.signal_connect_update_preedit_caret          (slot (panel_slot_update_preedit_caret));
    _panel_client.signal_connect_select_aux                    (slot (panel_slot_select_aux));
    _panel_client.signal_connect_select_candidate              (slot (panel_slot_select_candidate));
    _panel_client.signal_connect_process_key_event             (slot (panel_slot_process_key_event));
    _panel_client.signal_connect_commit_string                 (slot (panel_slot_commit_string));
    _panel_client.signal_connect_forward_key_event             (slot (panel_slot_forward_key_event));
    _panel_client.signal_connect_request_help                  (slot (panel_slot_request_help));
    _panel_client.signal_connect_request_factory_menu          (slot (panel_slot_request_factory_menu));
    _panel_client.signal_connect_change_factory                (slot (panel_slot_change_factory));
    _panel_client.signal_connect_reset_keyboard_ise            (slot (panel_slot_reset_keyboard_ise));
    _panel_client.signal_connect_update_keyboard_ise           (slot (panel_slot_update_keyboard_ise));
    _panel_client.signal_connect_show_preedit_string           (slot (panel_slot_show_preedit_string));
    _panel_client.signal_connect_hide_preedit_string           (slot (panel_slot_hide_preedit_string));
    _panel_client.signal_connect_update_preedit_string         (slot (panel_slot_update_preedit_string));
    _panel_client.signal_connect_get_surrounding_text          (slot (panel_slot_get_surrounding_text));
    _panel_client.signal_connect_delete_surrounding_text       (slot (panel_slot_delete_surrounding_text));
    _panel_client.signal_connect_update_displayed_candidate_number (slot (panel_slot_update_displayed_candidate_number));
    _panel_client.signal_connect_candidate_more_window_show    (slot (panel_slot_candidate_more_window_show));
    _panel_client.signal_connect_candidate_more_window_hide    (slot (panel_slot_candidate_more_window_hide));
    _panel_client.signal_connect_longpress_candidate           (slot (panel_slot_longpress_candidate));
    _panel_client.signal_connect_update_ise_input_context      (slot (panel_slot_update_ise_input_context));
    _panel_client.signal_connect_update_isf_candidate_panel    (slot (panel_slot_update_isf_candidate_panel));

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
        _used_ic_impl_list->si->set_frontend_data (static_cast <void*> (_used_ic_impl_list->parent));
        isf_wsc_context_del (_used_ic_impl_list->parent);
    }

    delete_all_ic_impl ();

    _fallback_instance.reset ();
    _fallback_factory.reset ();

    SCIM_DEBUG_FRONTEND(2) << " Releasing BackEnd...\n";
    _backend.reset ();

    SCIM_DEBUG_FRONTEND(2) << " Releasing Config...\n";
    _config.reset ();
    _config_connection.disconnect ();

    _focused_ic = NULL;
    _ic_list = NULL;

    _scim_initialized = false;

    _panel_client.reset_signal_handler ();
    panel_finalize ();
}

static void
_display_input_language (WSCContextISF *ic)
{
}

static void
open_next_factory (WSCContextISF *ic)
{
    if (!check_valid_ic (ic))
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
        _panel_client.register_input_context (ic->id, sf->get_uuid ());
        _panel_client.set_candidate_style (ic->id, ONE_LINE_CANDIDATE, FIXED_CANDIDATE_WINDOW);
        set_ic_capabilities (ic);
        turn_on_ic (ic);

        if (_shared_input_method) {
            _default_instance = ic->impl->si;
            ic->impl->shared_si = true;
        }
        //_popup_message (utf8_wcstombs (sf->get_name ()).c_str ());
    }
}

static void
open_previous_factory (WSCContextISF *ic)
{
    if (!check_valid_ic (ic))
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
        _panel_client.register_input_context (ic->id, sf->get_uuid ());
        _panel_client.set_candidate_style (ic->id, ONE_LINE_CANDIDATE, FIXED_CANDIDATE_WINDOW);
        set_ic_capabilities (ic);
        turn_on_ic (ic);

        if (_shared_input_method) {
            _default_instance = ic->impl->si;
            ic->impl->shared_si = true;
        }
        //_popup_message (utf8_wcstombs (sf->get_name ()).c_str ());
    }
}

static void
open_specific_factory (WSCContextISF *ic,
                       const String     &uuid)
{
    if (!check_valid_ic (ic))
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
        _panel_client.register_input_context (ic->id, sf->get_uuid ());
        set_ic_capabilities (ic);
        turn_on_ic (ic);

        if (_shared_input_method) {
            _default_instance = ic->impl->si;
            ic->impl->shared_si = true;
        }
    } else {
        std::cerr << "open_specific_factory () is failed!!!!!!\n";
        LOGE ("open_specific_factory () is failed. ic : %x uuid : %s", ic->id, uuid.c_str ());

        // turn_off_ic comment out panel_req_update_factory_info ()
        //turn_off_ic (ic);
        if (ic && ic->impl->is_on) {
            ic->impl->is_on = false;

            if (ic == _focused_ic) {
                ic->impl->si->focus_out ();

                panel_req_update_factory_info (ic);
                _panel_client.turn_off (ic->id);
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
    }
    else if (keyname >= 'A' && keyname <= 'Z') {
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
        case '\:':
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
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    uint32_t time = 0;
    uint32_t sym = 0;

    if (!fake)
        time = get_time ();

    uint32_t modifiers = 0;
    if (key.is_shift_down ())
        modifiers |= MOD_SHIFT_MASK;
    if (key.is_alt_down ())
        modifiers |= MOD_ALT_MASK;
    if (key.is_control_down ())
        modifiers |= MOD_CONTROL_MASK;

    sym = _keyname_to_keysym (key.code, &modifiers);

    if (ic)
        wsc_context_send_key (ic->ctx, key.code, modifiers, time, key.is_key_press ());
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

    si->signal_connect_expand_candidate (
        slot (slot_expand_candidate));
    si->signal_connect_contract_candidate (
        slot (slot_contract_candidate));

    si->signal_connect_set_candidate_style (
        slot (slot_set_candidate_style));
}

// Implementation of slot functions
static void
slot_show_preedit_string (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic) {
        if (ic->impl->use_preedit) {
            if (!ic->impl->preedit_started) {
                if (check_valid_ic (ic))
                    ic->impl->preedit_started = true;
            }
        } else {
            _panel_client.show_preedit_string (ic->id);
        }
    }
}

static void
slot_show_aux_string (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.show_aux_string (ic->id);
}

static void
slot_show_lookup_table (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.show_lookup_table (ic->id);
}

static void
slot_hide_preedit_string (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

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
            wsc_context_send_preedit_string (ic->ctx);
        } else {
            _panel_client.hide_preedit_string (ic->id);
        }
    }
}

static void
slot_hide_aux_string (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.hide_aux_string (ic->id);
}

static void
slot_hide_lookup_table (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.hide_lookup_table (ic->id);
}

static void
slot_update_preedit_caret (IMEngineInstanceBase *si, int caret)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic && ic->impl->preedit_caret != caret) {
        ic->impl->preedit_caret = caret;
        if (ic->impl->use_preedit) {
            if (!ic->impl->preedit_started) {
                if (!check_valid_ic (ic))
                    return;

                ic->impl->preedit_started = true;
            }
        } else {
            _panel_client.update_preedit_caret (ic->id, caret);
        }
    }
}

static void
slot_update_preedit_string (IMEngineInstanceBase *si,
                            const WideString & str,
                            const AttributeList & attrs,
                            int caret)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

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
            wsc_context_send_preedit_string (ic->ctx);
        } else {
            _panel_client.update_preedit_string (ic->id, str, attrs, caret);
        }
    }
}

static void
slot_update_aux_string (IMEngineInstanceBase *si,
                        const WideString & str,
                        const AttributeList & attrs)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.update_aux_string (ic->id, str, attrs);
}

static void
slot_commit_string (IMEngineInstanceBase *si,
                    const WideString & str)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    if (ic && ic->ctx) {
        if (utf8_wcstombs (str) == String (" ") || utf8_wcstombs (str) == String ("　"))
            autoperiod_insert (ic);

        Eina_Bool auto_capitalized = EINA_FALSE;

        if (ic->impl) {
            if (wsc_context_input_panel_layout_get (ic->ctx) == ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL &&
                ic->impl->shift_mode_enabled &&
                ic->impl->autocapital_type != ECORE_IMF_AUTOCAPITAL_TYPE_NONE &&
                hw_keyboard_num_get () == 0) {
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

                    wsc_context_commit_string (ic->ctx, converted);
                    auto_capitalized = EINA_TRUE;
                }
            }
        }

        if (!auto_capitalized) {
            wsc_context_commit_string (ic->ctx, utf8_wcstombs (str).c_str ());
        }
    }
}

static void
slot_forward_key_event (IMEngineInstanceBase *si,
                        const KeyEvent & key)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

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
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.update_lookup_table (ic->id, table);
}

static void
slot_register_properties (IMEngineInstanceBase *si,
                          const PropertyList & properties)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.register_properties (ic->id, properties);
}

static void
slot_update_property (IMEngineInstanceBase *si,
                      const Property & property)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.update_property (ic->id, property);
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
    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " helper= " << helper_uuid << " context="
                           << (ic != NULL ? ic->id : -1) << " ic=" << ic
                           << " ic-uuid=" << ((ic != NULL && ic->impl != NULL) ? ic->impl->si->get_factory_uuid () : "") << "...\n";

    if (ic && ic->impl)
        _panel_client.start_helper (ic->id, helper_uuid);
}

static void
slot_stop_helper (IMEngineInstanceBase *si,
                  const String &helper_uuid)
{
    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " helper= " << helper_uuid << " context=" << (ic != NULL ? ic->id : -1) << " ic=" << ic << "...\n";

    if (ic && ic->impl)
        _panel_client.stop_helper (ic->id, helper_uuid);
}

static void
slot_send_helper_event (IMEngineInstanceBase *si,
                        const String      &helper_uuid,
                        const Transaction &trans)
{
    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " helper= " << helper_uuid << " context="
                           << (ic != NULL ? ic->id : -1) << " ic=" << ic
                           << " ic-uuid=" << ((ic != NULL && ic->impl != NULL) ? ic->impl->si->get_factory_uuid () : "") << "...\n";

    if (ic && ic->impl)
        _panel_client.send_helper_event (ic->id, helper_uuid, trans);
}

static bool
slot_get_surrounding_text (IMEngineInstanceBase *si,
                           WideString            &text,
                           int                   &cursor,
                           int                    maxlen_before,
                           int                    maxlen_after)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic) {
        char *surrounding = NULL;
        int   cursor_index;
        if (wsc_context_surrounding_get (_focused_ic->ctx, &surrounding, &cursor_index)) {
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
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic) {
        wsc_context_delete_surrounding (_focused_ic->ctx, offset, len);
        return true;
    }
    return false;
}

static void
slot_expand_candidate (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.expand_candidate (ic->id);
}

static void
slot_contract_candidate (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.contract_candidate (ic->id);
}

static void
slot_set_candidate_style (IMEngineInstanceBase *si, ISF_CANDIDATE_PORTRAIT_LINE_T portrait_line, ISF_CANDIDATE_MODE_T mode)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    WSCContextISF *ic = static_cast<WSCContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.set_candidate_style (ic->id, portrait_line, mode);
}

static void
reload_config_callback (const ConfigPointer &config)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

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

    // Get keyboard layout setting
    // Flush the global config first, in order to load the new configs from disk.
    scim_global_config_flush ();

    _keyboard_layout = scim_get_default_keyboard_layout ();
}

static void
fallback_commit_string_cb (IMEngineInstanceBase  *si,
                           const WideString      &str)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (_focused_ic && _focused_ic->impl) {
        wsc_context_commit_string (_focused_ic->ctx, utf8_wcstombs (str).c_str ());
    }
}

