/*
 * Copyright © 2012, 2013 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "scim_private.h"
#include <unistd.h>

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_Input.h>
#include <Ecore_Wayland.h>
#include <dlog.h>
#ifdef HAVE_VCONF
#include <vconf.h>
#endif
#include "isf_debug.h"

#include "wayland_imcontext.h"

#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG "IMMODULE"

#define HIDE_TIMER_INTERVAL     0.05
#define WAIT_FOR_FILTER_DONE_SECOND 2

#define MOD_SHIFT_MASK      0x01
#define MOD_ALT_MASK        0x02
#define MOD_CONTROL_MASK    0x04

static Eina_Bool _clear_hide_timer();
static Ecore_Timer *_hide_timer  = NULL;

// TIZEN_ONLY(20150708): Support back key
#define BACK_KEY "XF86Back"

static Eina_Bool             input_detected = EINA_FALSE;
static Eina_Bool             will_hide = EINA_FALSE;

static Ecore_Event_Filter   *_ecore_event_filter_handler = NULL;
static Ecore_IMF_Context    *_focused_ctx                = NULL;
static Ecore_IMF_Context    *_show_req_ctx               = NULL;
static Ecore_IMF_Context    *_hide_req_ctx               = NULL;

static Eina_Rectangle        _keyboard_geometry = {0, 0, 0, 0};

static Ecore_IMF_Input_Panel_State _input_panel_state    = ECORE_IMF_INPUT_PANEL_STATE_HIDE;
static Ecore_Event_Handler *_win_focus_out_handler       = NULL;
static Ecore_Event_Handler *_conformant_change_handler   = NULL;

static Eina_Bool             received_will_hide_event    = EINA_FALSE;
static Eina_Bool             conformant_reset_done       = EINA_FALSE;
//

struct _WaylandIMContext
{
    Ecore_IMF_Context *ctx;

    struct wl_text_input_manager *text_input_manager;
    struct wl_text_input *text_input;

    Ecore_Wl_Window *window;
    Ecore_Wl_Input  *input;
    Evas            *canvas;

    char *preedit_text;
    char *preedit_commit;
    char *language;
    Eina_List *preedit_attrs;
    int32_t preedit_cursor;

    struct
    {
        Eina_List *attrs;
        int32_t cursor;
    } pending_preedit;

    int32_t cursor_position;

    struct
    {
        int x;
        int y;
        int width;
        int height;
    } cursor_location;

    xkb_mod_mask_t control_mask;
    xkb_mod_mask_t alt_mask;
    xkb_mod_mask_t shift_mask;

    uint32_t serial;
    uint32_t content_purpose;
    uint32_t content_hint;
    Ecore_IMF_Input_Panel_Layout input_panel_layout;

    // TIZEN_ONLY(20150716): Support return key type
    uint32_t return_key_type;

    Eina_Bool return_key_disabled;

    void *imdata;
    uint32_t imdata_size;

    uint32_t bidi_direction;

    void *input_panel_data;
    uint32_t input_panel_data_length;

    struct
    {
        uint32_t serial;
        uint32_t state;
    } last_key_event_filter;
    Eina_List *keysym_list;

    uint32_t last_reset_serial;
    //
};

// TIZEN_ONLY(20150708): Support back key
static void _input_panel_hide(Ecore_IMF_Context *ctx, Eina_Bool instant);

static Ecore_IMF_Context *
get_using_ctx ()
{
    return (_show_req_ctx ? _show_req_ctx : _focused_ctx);
}

static Eina_Bool
key_down_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
    Ecore_Event_Key *ev = (Ecore_Event_Key *)event;
    if (!ev || !ev->keyname) return EINA_TRUE;

    if ((_input_panel_state == ECORE_IMF_INPUT_PANEL_STATE_SHOW ||
         _input_panel_state == ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW) &&
         strcmp(ev->keyname, BACK_KEY) == 0)
        return EINA_FALSE;

    return EINA_TRUE;
}

static Eina_Bool
key_up_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
    Ecore_Event_Key *ev = (Ecore_Event_Key *)event;
    if (!ev || !ev->keyname) return EINA_TRUE;

    Ecore_IMF_Context *active_ctx = get_using_ctx ();

    if (!active_ctx) return EINA_TRUE;

    if (_input_panel_state == ECORE_IMF_INPUT_PANEL_STATE_HIDE ||
        strcmp(ev->keyname, BACK_KEY) != 0)
        return EINA_TRUE;

    ecore_imf_context_reset(active_ctx);

    _input_panel_hide(active_ctx, EINA_TRUE);

    return EINA_FALSE;
}

#ifdef _WEARABLE
static Eina_Bool
rotary_event_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
    Ecore_Event_Detent_Rotate *ev = event;
    if (!ev) return EINA_TRUE;

    Ecore_IMF_Context *active_ctx = NULL;
    if (_show_req_ctx)
        active_ctx = _show_req_ctx;
    else if (_focused_ctx)
        active_ctx = _focused_ctx;

    if (!active_ctx) return EINA_TRUE;

    if (_input_panel_state == ECORE_IMF_INPUT_PANEL_STATE_HIDE)
        return EINA_TRUE;

    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(active_ctx);
    if (imcontext) {
        wl_text_input_process_input_device_event(imcontext->text_input,
            (unsigned int)ECORE_EVENT_DETENT_ROTATE, (char*)event, sizeof(Ecore_Event_Detent_Rotate));
    }

    return EINA_FALSE;
}
#endif

static Eina_Bool
_ecore_event_filter_cb(void *data, void *loop_data EINA_UNUSED, int type, void *event)
{
    if (type == ECORE_EVENT_KEY_DOWN) {
        return key_down_cb(data, type, event);
    }
    else if (type == ECORE_EVENT_KEY_UP) {
        return key_up_cb(data, type, event);
    }
#ifdef _WEARABLE
    /* The IME needs to process Rotary event prior to client application */
    else if (type == ECORE_EVENT_DETENT_ROTATE) {
        return rotary_event_cb(data, type, event);
    }
#endif

    return EINA_TRUE;
}

static void
register_key_handler()
{
    if (!_ecore_event_filter_handler)
        _ecore_event_filter_handler = ecore_event_filter_add(NULL, _ecore_event_filter_cb, NULL, NULL);
}

static void
unregister_key_handler()
{
    if (_ecore_event_filter_handler) {
        ecore_event_filter_del(_ecore_event_filter_handler);
        _ecore_event_filter_handler = NULL;
    }

    _clear_hide_timer();
}
//

static Eina_Bool
_clear_hide_timer()
{
    if (_hide_timer) {
        ecore_timer_del(_hide_timer);
        _hide_timer = NULL;
        return EINA_TRUE;
    }

    return EINA_FALSE;
}

static void _win_focus_out_handler_del ()
{
    if (_win_focus_out_handler) {
        ecore_event_handler_del (_win_focus_out_handler);
        _win_focus_out_handler = NULL;
    }
}

static void _conformant_change_handler_del()
{
    if (_conformant_change_handler) {
        ecore_event_handler_del(_conformant_change_handler);
        _conformant_change_handler = NULL;
    }
}

static void
_send_input_panel_hide_request(Ecore_IMF_Context *ctx)
{
    LOGD ("");
    // TIZEN_ONLY(20150708): Support back key
    _hide_req_ctx = NULL;
    //

    _win_focus_out_handler_del ();

    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (imcontext && imcontext->text_input) {
        wl_text_input_hide_input_panel(imcontext->text_input);
    } else {
        LOGD("creating temporary context for sending hide request\n");
        const char *ctx_id = ecore_imf_context_default_id_get();
        Ecore_IMF_Context *temp_context = ecore_imf_context_add(ctx_id);
        if (temp_context) {
            imcontext = (WaylandIMContext *)ecore_imf_context_data_get(temp_context);
            if (imcontext) wl_text_input_hide_input_panel(imcontext->text_input);
            ecore_imf_context_del(temp_context);
        }
    }
}

static Eina_Bool
_hide_timer_handler(void *data)
{
    Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)data;
    _send_input_panel_hide_request(ctx);

    _hide_timer = NULL;
    return ECORE_CALLBACK_CANCEL;
}

static void
_input_panel_hide_timer_start(void *data)
{
    if (!_hide_timer)
        _hide_timer = ecore_timer_add(HIDE_TIMER_INTERVAL, _hide_timer_handler, data);
}

static void
_input_panel_hide(Ecore_IMF_Context *ctx, Eina_Bool instant)
{
    LOGD ("ctx : %p", ctx);

    if (!get_using_ctx()) {
        LOGW("Can't hide input_panel because there is no using context!!");
        return;
    }

    will_hide = EINA_TRUE;

    if (instant || (_hide_timer && ecore_timer_pending_get(_hide_timer) <= 0.0)) {
        _clear_hide_timer();
        _send_input_panel_hide_request(ctx);
    }
    else {
        _input_panel_hide_timer_start(ctx);
        // TIZEN_ONLY(20150708): Support back key
        _hide_req_ctx = ctx;
        //
    }
}

static unsigned int
utf8_offset_to_characters(const char *str, int offset)
{
    int index = 0;
    unsigned int i = 0;

    for (; index < offset; i++) {
        if (eina_unicode_utf8_next_get(str, &index) == 0)
            break;
    }

    return i;
}

static void
send_cursor_location(WaylandIMContext *imcontext)
{
    Ecore_Evas *ee = NULL;
    int canvas_x = 0, canvas_y = 0;

    if (imcontext->canvas) {
        ee = ecore_evas_ecore_evas_get(imcontext->canvas);
        if (ee)
            ecore_evas_geometry_get(ee, &canvas_x, &canvas_y, NULL, NULL);
    }

    if (imcontext->text_input) {
        wl_text_input_set_cursor_rectangle(imcontext->text_input,
                imcontext->cursor_location.x + canvas_x,
                imcontext->cursor_location.y + canvas_y,
                imcontext->cursor_location.width,
                imcontext->cursor_location.height);
    }
}

static void
update_state(WaylandIMContext *imcontext)
{
    if (!imcontext->ctx)
        return;

    send_cursor_location (imcontext);

    if (imcontext->text_input) {
        wl_text_input_commit_state(imcontext->text_input, ++imcontext->serial);
    }
}

static void
clear_preedit(WaylandIMContext *imcontext)
{
    Ecore_IMF_Preedit_Attr *attr = NULL;

    imcontext->preedit_cursor = 0;

    if (imcontext->preedit_text) {
        free(imcontext->preedit_text);
        imcontext->preedit_text = NULL;
    }

    if (imcontext->preedit_commit) {
        free(imcontext->preedit_commit);
        imcontext->preedit_commit = NULL;
    }

    if (imcontext->preedit_attrs) {
        EINA_LIST_FREE(imcontext->preedit_attrs, attr)
            free(attr);
    }

    imcontext->preedit_attrs = NULL;
}

static void
text_input_commit_string(void                 *data,
                         struct wl_text_input *text_input EINA_UNUSED,
                         uint32_t              serial,
                         const char           *text)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)data;
    Eina_Bool old_preedit = EINA_FALSE;

    LOGD("commit event (text: '%s', current pre-edit: '%s')",
            text,
            imcontext->preedit_text ? imcontext->preedit_text : "");

    old_preedit =
        imcontext->preedit_text && strlen(imcontext->preedit_text) > 0;

    if (!imcontext->ctx)
        return;

    if (old_preedit)
    {
        ecore_imf_context_preedit_end_event_add(imcontext->ctx);
        ecore_imf_context_event_callback_call(imcontext->ctx,
                ECORE_IMF_CALLBACK_PREEDIT_END,
                NULL);
    }

    clear_preedit(imcontext);

    ecore_imf_context_commit_event_add(imcontext->ctx, text);
    ecore_imf_context_event_callback_call(imcontext->ctx, ECORE_IMF_CALLBACK_COMMIT, (void *)text);
}

static void
commit_preedit(WaylandIMContext *imcontext)
{
    if (!imcontext->preedit_commit)
        return;

    if (!imcontext->ctx)
        return;

    ecore_imf_context_preedit_changed_event_add(imcontext->ctx);
    ecore_imf_context_event_callback_call(imcontext->ctx,
            ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
            NULL);

    ecore_imf_context_preedit_end_event_add(imcontext->ctx);
    ecore_imf_context_event_callback_call(imcontext->ctx,
            ECORE_IMF_CALLBACK_PREEDIT_END, NULL);

    ecore_imf_context_commit_event_add(imcontext->ctx,
            imcontext->preedit_commit);
    ecore_imf_context_event_callback_call(imcontext->ctx,
            ECORE_IMF_CALLBACK_COMMIT,
            (void *)imcontext->preedit_commit);
}

static void
set_focus(Ecore_IMF_Context *ctx)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext || !imcontext->window) return;

    Ecore_Wl_Input *input = ecore_wl_window_keyboard_get(imcontext->window);
    if (!input)
        return;

    struct wl_seat *seat = ecore_wl_input_seat_get(input);
    if (!seat)
        return;

    imcontext->input = input;

    wl_text_input_activate(imcontext->text_input, seat,
            ecore_wl_window_surface_get(imcontext->window));
}

// TIZEN_ONLY(20160217): ignore the duplicate show request
static Eina_Bool _compare_context(Ecore_IMF_Context *ctx1, Ecore_IMF_Context *ctx2)
{
    if (!ctx1 || !ctx2) return EINA_FALSE;

    if ((ecore_imf_context_autocapital_type_get(ctx1) == ecore_imf_context_autocapital_type_get(ctx2)) &&
        (ecore_imf_context_input_panel_layout_get(ctx1) == ecore_imf_context_input_panel_layout_get(ctx2)) &&
        (ecore_imf_context_input_panel_layout_variation_get(ctx1) == ecore_imf_context_input_panel_layout_variation_get(ctx2)) &&
        (ecore_imf_context_input_panel_language_get(ctx1) == ecore_imf_context_input_panel_language_get(ctx2)) &&
        (ecore_imf_context_input_panel_return_key_type_get(ctx1) == ecore_imf_context_input_panel_return_key_type_get(ctx2)) &&
        (ecore_imf_context_input_panel_return_key_disabled_get(ctx1) == ecore_imf_context_input_panel_return_key_disabled_get(ctx2)) &&
        (ecore_imf_context_input_panel_caps_lock_mode_get(ctx1) == ecore_imf_context_input_panel_caps_lock_mode_get(ctx2)))
        return EINA_TRUE;

    return EINA_FALSE;
}
//

static void _send_will_hide_ack(WaylandIMContext *imcontext)
{
    if (!imcontext) return;
    if (!(imcontext->text_input)) return;

    const char *szWillHideAck = "WILL_HIDE_ACK";
    wl_text_input_set_input_panel_data(imcontext->text_input, szWillHideAck, strlen(szWillHideAck));
    received_will_hide_event = EINA_FALSE;
}

static Eina_Bool _client_window_focus_out_cb(void *data, int ev_type, void *ev)
{
    Ecore_Wl_Event_Focus_Out *e = (Ecore_Wl_Event_Focus_Out *)ev;
    Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)data;
    if (!ctx || !e) return ECORE_CALLBACK_PASS_ON;

    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get (ctx);
    if (!imcontext) return ECORE_CALLBACK_PASS_ON;

    unsigned int client_win_id = ecore_wl_window_id_get (imcontext->window);

    LOGD ("ctx : %p, client_window id : %#x, focus-out win : %#x\n", ctx, client_win_id, e->win);

    if (client_win_id > 0) {
        if (e->win == client_win_id) {
            LOGD ("window focus out\n");

            if (_focused_ctx == ctx) {
                wayland_im_context_focus_out (ctx);
            }

            if (_show_req_ctx == ctx) {
                _input_panel_hide (ctx, EINA_TRUE);
            }

            /* If our window loses focus, send WILL_HIDE_ACK right away */
            _send_will_hide_ack(imcontext);
        }
    }
    else {
        _input_panel_hide (ctx, EINA_FALSE);
    }

    return ECORE_CALLBACK_PASS_ON;
}

static void send_will_hide_ack(Ecore_IMF_Context *ctx)
{
    Eina_Bool need_temporary_context = EINA_FALSE;
    WaylandIMContext *imcontext = NULL;

    if (!ctx) {
        LOGD("ctx is NULL\n");
        need_temporary_context = EINA_TRUE;
    } else {
        imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
        if (!imcontext) {
            LOGD("imcontext is NULL\n");
            need_temporary_context = EINA_TRUE;
        }
    }

    /* When the RENDER_POST event is emitted, it is possible that our IMF_Context is already deleted,
       meaning that there is no connection available for communicating with the window manager.
       So we are creating a temporary context for sending WILL_HIDE_ACK message */
    if (need_temporary_context) {
        LOGD("creating temporary context for sending WILL_HIDE_ACK\n");
        const char *ctx_id = ecore_imf_context_default_id_get();
        Ecore_IMF_Context *temp_context = ecore_imf_context_add(ctx_id);
        if (temp_context) {
            imcontext = (WaylandIMContext *)ecore_imf_context_data_get(temp_context);
            if (imcontext) _send_will_hide_ack(imcontext);
            ecore_imf_context_del(temp_context);
        }
        return;
    }

    if (ctx && imcontext) {
        if (ecore_imf_context_client_canvas_get(ctx) && ecore_wl_window_conformant_get(imcontext->window)) {
            if (conformant_reset_done && received_will_hide_event) {
                LOGD("Send will hide ack, conformant_reset_done = 1, received_will_hide_event = 1\n");
                _send_will_hide_ack(imcontext);
                conformant_reset_done = EINA_FALSE;
                received_will_hide_event = EINA_FALSE;
            } else {
                LOGD ("conformant_reset_done=%d, received_will_hide_event=%d\n",
                    conformant_reset_done, received_will_hide_event);
            }
        } else {
            _send_will_hide_ack (imcontext);
            LOGD("Send will hide ack, since there is no conformant available\n");
        }
    }
}

static void _render_post_cb(void *data, Evas *e, void *event_info)
{
    Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)data;
    if (!ctx) return;

    void *callback = evas_event_callback_del(e, EVAS_CALLBACK_RENDER_POST, _render_post_cb);
    LOGD("[_render_post_cb], conformant_reset_done = 1 , %p\n", callback);
    conformant_reset_done = EINA_TRUE;
    send_will_hide_ack(ctx);
}

static Eina_Bool _conformant_change_cb(void *data, int ev_type, void *ev)
{
    Ecore_Wl_Event_Conformant_Change *e = (Ecore_Wl_Event_Conformant_Change *)ev;
    Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)data;
    if (!e || !ctx) return ECORE_CALLBACK_PASS_ON;

    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return ECORE_CALLBACK_PASS_ON;

    LOGD ("CONFORMANT changed!! : %d %d", e->part_type, e->state);

    Evas *evas = (Evas*)ecore_imf_context_client_canvas_get(ctx);
    if (e->state == 0) {
        LOGD("conformant_reset_done = 0, registering _render_post_cb\n");
        conformant_reset_done = EINA_FALSE;
        if (evas && ecore_wl_window_conformant_get(imcontext->window)) {
            evas_event_callback_add(evas, EVAS_CALLBACK_RENDER_POST, _render_post_cb, ctx);
        }
    } else {
        conformant_reset_done = EINA_FALSE;
        if (evas) {
            evas_event_callback_del(evas, EVAS_CALLBACK_RENDER_POST, _render_post_cb);
        }
    }

    return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
show_input_panel(Ecore_IMF_Context *ctx)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

    char *surrounding = NULL;
    int cursor_pos;

    if ((!imcontext) || (!imcontext->window) || (!imcontext->text_input))
        return EINA_FALSE;

    if (input_detected)
        return EINA_FALSE;

    if (!imcontext->input)
        set_focus(ctx);

    _clear_hide_timer();

    _win_focus_out_handler_del ();
    _win_focus_out_handler = ecore_event_handler_add (ECORE_WL_EVENT_FOCUS_OUT, _client_window_focus_out_cb, ctx);
    _conformant_change_handler_del ();
    _conformant_change_handler = ecore_event_handler_add(ECORE_WL_EVENT_CONFORMANT_CHANGE, _conformant_change_cb, ctx);

    // TIZEN_ONLY(20160217): ignore the duplicate show request
    if ((_show_req_ctx == ctx) && _compare_context(_show_req_ctx, ctx) && (!will_hide)) {
        if (_input_panel_state == ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW ||
            _input_panel_state == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
            LOGD("already show. ctx : %p", ctx);

            return EINA_FALSE;
        }
    }

    will_hide = EINA_FALSE;
    _show_req_ctx = ctx;
    //

    // TIZEN_ONLY(20150715): Support input_panel_state_get
    _input_panel_state = ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW;

    int layout_variation = ecore_imf_context_input_panel_layout_variation_get (ctx);
    uint32_t new_purpose = 0;
    switch (imcontext->content_purpose) {
        case WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS:
            if (layout_variation == ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_SIGNED)
                new_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS_SIGNED;
            else if (layout_variation == ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_DECIMAL)
                new_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS_DECIMAL;
            else if (layout_variation == ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_SIGNED_AND_DECIMAL)
                new_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS_SIGNEDDECIMAL;
            else
                new_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_PASSWORD:
            if (layout_variation == ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD_VARIATION_NUMBERONLY)
                new_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_PASSWORD_DIGITS;
            else
                new_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_PASSWORD;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_NORMAL:
            if (layout_variation == ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL_VARIATION_FILENAME)
                new_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_FILENAME;
            else if (layout_variation == ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL_VARIATION_PERSON_NAME)
                new_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_NAME;
            else
                new_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_NORMAL;
            break;
        default :
            new_purpose = imcontext->content_purpose;
            break;
    }
    //

    wl_text_input_set_content_type(imcontext->text_input,
            imcontext->content_hint,
            new_purpose);

    if (ecore_imf_context_surrounding_get(imcontext->ctx, &surrounding, &cursor_pos)) {
        if (surrounding)
            free (surrounding);

        imcontext->cursor_position = cursor_pos;
        wl_text_input_set_cursor_position(imcontext->text_input, cursor_pos);
    }
    // TIZEN_ONLY(20150716): Support return key type
    wl_text_input_set_return_key_type(imcontext->text_input,
            imcontext->return_key_type);

    wl_text_input_set_return_key_disabled(imcontext->text_input,
            imcontext->return_key_disabled);

    if (imcontext->imdata_size > 0)
        wl_text_input_set_input_panel_data(imcontext->text_input, (const char *)imcontext->imdata, imcontext->imdata_size);

    SECURE_LOGD ("ctx : %p, layout : %d, layout variation : %d\n", ctx,
            ecore_imf_context_input_panel_layout_get (ctx),
            layout_variation);
    SECURE_LOGD ("language : %d, cursor position : %d\n",
            ecore_imf_context_input_panel_language_get (ctx),
            cursor_pos);
    SECURE_LOGD ("return key type : %d, return key disabled : %d, autocapital type : %d\n",
            ecore_imf_context_input_panel_return_key_type_get (ctx),
            ecore_imf_context_input_panel_return_key_disabled_get (ctx),
            ecore_imf_context_autocapital_type_get (ctx));
    //

    wl_text_input_show_input_panel(imcontext->text_input);

    return EINA_TRUE;
}

static void
text_input_preedit_string(void                 *data,
                          struct wl_text_input *text_input EINA_UNUSED,
                          uint32_t              serial,
                          const char           *text,
                          const char           *commit)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)data;
    Eina_Bool old_preedit = EINA_FALSE;

    LOGD("preedit event (text: '%s', current pre-edit: '%s')",
            text,
            imcontext->preedit_text ? imcontext->preedit_text : "");

    old_preedit =
        imcontext->preedit_text && strlen(imcontext->preedit_text) > 0;

    clear_preedit(imcontext);

    imcontext->preedit_text = strdup(text);
    imcontext->preedit_commit = strdup(commit);
    imcontext->preedit_cursor =
        utf8_offset_to_characters(text, imcontext->pending_preedit.cursor);
    imcontext->preedit_attrs = imcontext->pending_preedit.attrs;

    imcontext->pending_preedit.attrs = NULL;

    if (!old_preedit) {
        ecore_imf_context_preedit_start_event_add(imcontext->ctx);
        ecore_imf_context_event_callback_call(imcontext->ctx,
                ECORE_IMF_CALLBACK_PREEDIT_START,
                NULL);
    }

    ecore_imf_context_preedit_changed_event_add(imcontext->ctx);
    ecore_imf_context_event_callback_call(imcontext->ctx,
            ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
            NULL);

    if (imcontext->preedit_text && strlen(imcontext->preedit_text) == 0) {
        ecore_imf_context_preedit_end_event_add(imcontext->ctx);
        ecore_imf_context_event_callback_call(imcontext->ctx,
                ECORE_IMF_CALLBACK_PREEDIT_END,
                NULL);
    }
}

static void
text_input_delete_surrounding_text(void                 *data,
                                   struct wl_text_input *text_input EINA_UNUSED,
                                   int32_t               index,
                                   uint32_t              length)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)data;
    Ecore_IMF_Event_Delete_Surrounding ev;
    LOGD("delete surrounding text (index: %d, length: %u)",
            index, length);

    ev.offset = index;
    ev.n_chars = length;

    ecore_imf_context_delete_surrounding_event_add(imcontext->ctx, ev.offset, ev.n_chars);
    ecore_imf_context_event_callback_call(imcontext->ctx, ECORE_IMF_CALLBACK_DELETE_SURROUNDING, &ev);
}

static void
text_input_cursor_position(void                 *data EINA_UNUSED,
                           struct wl_text_input *text_input EINA_UNUSED,
                           int32_t               index,
                           int32_t               anchor)
{
    LOGD("cursor_position for next commit (index: %d, anchor: %d)",
            index, anchor);
}

static void
text_input_preedit_styling(void                 *data,
                           struct wl_text_input *text_input EINA_UNUSED,
                           uint32_t              index,
                           uint32_t              length,
                           uint32_t              style)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)data;
    Ecore_IMF_Preedit_Attr *attr = calloc(1, sizeof(*attr));

    switch (style)
    {
        case WL_TEXT_INPUT_PREEDIT_STYLE_DEFAULT:
            attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB1;
            break;
        case WL_TEXT_INPUT_PREEDIT_STYLE_UNDERLINE:
            attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB1;
            break;
        case WL_TEXT_INPUT_PREEDIT_STYLE_INCORRECT:
            break;
        case WL_TEXT_INPUT_PREEDIT_STYLE_HIGHLIGHT:
            attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB3;
            break;
        case WL_TEXT_INPUT_PREEDIT_STYLE_ACTIVE:
            break;
        case WL_TEXT_INPUT_PREEDIT_STYLE_INACTIVE:
            break;
        case WL_TEXT_INPUT_PREEDIT_STYLE_SELECTION:
            attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB2;
            break;
        default:
            attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB1;
            break;
    }

    attr->start_index = index;
    attr->end_index = index + length;

    imcontext->pending_preedit.attrs =
        eina_list_append(imcontext->pending_preedit.attrs, attr);
}

static void
text_input_preedit_cursor(void                 *data,
                          struct wl_text_input *text_input EINA_UNUSED,
                          int32_t               index)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)data;

    imcontext->pending_preedit.cursor = index;
}

static xkb_mod_index_t
modifiers_get_index(struct wl_array *modifiers_map, const char *name)
{
    xkb_mod_index_t index = 0;
    char *p = modifiers_map->data;

    while ((const char *)p < ((const char *)modifiers_map->data + modifiers_map->size))
    {
        if (strcmp(p, name) == 0)
            return index;

        index++;
        p += strlen(p) + 1;
    }

    return XKB_MOD_INVALID;
}

static xkb_mod_mask_t
modifiers_get_mask(struct wl_array *modifiers_map,
        const char *name)
{
    xkb_mod_index_t index = modifiers_get_index(modifiers_map, name);

    if (index == XKB_MOD_INVALID)
        return XKB_MOD_INVALID;

    return 1 << index;
}

static void
text_input_modifiers_map(void                 *data,
                         struct wl_text_input *text_input EINA_UNUSED,
                         struct wl_array      *map)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)data;
    LOGD ("");
    imcontext->shift_mask = modifiers_get_mask(map, "Shift");
    imcontext->control_mask = modifiers_get_mask(map, "Control");
    imcontext->alt_mask = modifiers_get_mask(map, "Mod1");
}

static void
text_input_keysym(void                 *data,
                  struct wl_text_input *text_input EINA_UNUSED,
                  uint32_t              serial EINA_UNUSED,
                  uint32_t              time,
                  uint32_t              sym,
                  uint32_t              state,
                  uint32_t              modifiers)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)data;
    char string[32], key[32], keyname[32];
    Ecore_Event_Key *e;

    memset(key, 0, sizeof(key));
    xkb_keysym_get_name(sym, key, sizeof(key));

    memset(keyname, 0, sizeof(keyname));
    xkb_keysym_get_name(sym, keyname, sizeof(keyname));
    if (keyname[0] == '\0')
        snprintf(keyname, sizeof(keyname), "Keysym-%u", sym);

    memset(string, 0, sizeof(string));
    xkb_keysym_to_utf8(sym, string, 32);

    LOGD("key event (key: %s)", keyname);

    e = calloc(1, sizeof(Ecore_Event_Key) + strlen(key) + strlen(keyname) +
            strlen(string) + 3);
    if (!e) return;

    e->keyname = (char *)(e + 1);
    e->key = e->keyname + strlen(keyname) + 1;
    e->string = e->key + strlen(key) + 1;
    e->compose = e->string;

    strncpy((char *)e->keyname, keyname, strlen(keyname));
    strncpy((char *)e->key, key, strlen(key));
    strncpy((char *)e->string, string, strlen(string));

    e->window = ecore_wl_window_id_get(imcontext->window);
    e->event_window = ecore_wl_window_id_get(imcontext->window);
    e->timestamp = time;

    e->modifiers = 0;
    if (modifiers & imcontext->shift_mask)
        e->modifiers |= ECORE_EVENT_MODIFIER_SHIFT;

    if (modifiers & imcontext->control_mask)
        e->modifiers |= ECORE_EVENT_MODIFIER_CTRL;

    if (modifiers & imcontext->alt_mask)
        e->modifiers |= ECORE_EVENT_MODIFIER_ALT;
    //Save "wl_text_input::keysym" keysym to list if list is not empty,
    //if not, send keysym to ecore loop as key event.
    //This code let key event which will be filtered by IME one by one.
    if (eina_list_count(imcontext->keysym_list)) {
        e->data = (void *)(state ? ECORE_EVENT_KEY_DOWN : ECORE_EVENT_KEY_UP);
        imcontext->keysym_list = eina_list_prepend(imcontext->keysym_list, e);
    }
    else {
        if (state)
            ecore_event_add(ECORE_EVENT_KEY_DOWN, e, NULL, NULL);
        else
            ecore_event_add(ECORE_EVENT_KEY_UP, e, NULL, NULL);
    }
}

static void
text_input_enter(void                 *data,
                 struct wl_text_input *text_input EINA_UNUSED,
                 struct wl_surface    *surface EINA_UNUSED)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)data;

    update_state(imcontext);
}

static void
text_input_leave(void                 *data,
                 struct wl_text_input *text_input EINA_UNUSED)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)data;

    /* clear preedit */
    commit_preedit(imcontext);
    clear_preedit(imcontext);
}

static void
text_input_input_panel_state(void                 *data EINA_UNUSED,
                             struct wl_text_input *text_input EINA_UNUSED,
                             uint32_t              state EINA_UNUSED)
{
    // TIZEN_ONLY(20150708): Support input panel state callback
    WaylandIMContext *imcontext = (WaylandIMContext *)data;
    LOGD("input panel state: %d", state);
    switch (state) {
        case WL_TEXT_INPUT_INPUT_PANEL_STATE_HIDE:
            _input_panel_state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;
            if (imcontext->ctx == _show_req_ctx)
                _show_req_ctx = NULL;

            will_hide = EINA_FALSE;

            received_will_hide_event = EINA_TRUE;
            LOGD("received_will_hide_event = 1\n");
            break;
        case WL_TEXT_INPUT_INPUT_PANEL_STATE_SHOW:
            _input_panel_state = ECORE_IMF_INPUT_PANEL_STATE_SHOW;
            received_will_hide_event = EINA_FALSE;
            LOGD("received_will_hide_event = 0\n");
            break;
        default:
            _input_panel_state = (Ecore_IMF_Input_Panel_State)state;
            break;
    }

    ecore_imf_context_input_panel_event_callback_call(imcontext->ctx,
            ECORE_IMF_INPUT_PANEL_STATE_EVENT,
            _input_panel_state);

    if (state == WL_TEXT_INPUT_INPUT_PANEL_STATE_HIDE) {
        static Evas_Coord scr_w = 0, scr_h = 0;
        if (scr_w == 0 || scr_h == 0) {
            ecore_wl_sync();
            ecore_wl_screen_size_get(&scr_w, &scr_h);
        }
        _keyboard_geometry.x = 0;
        _keyboard_geometry.y = scr_h;
        _keyboard_geometry.w = 0;
        _keyboard_geometry.h = 0;
        ecore_imf_context_input_panel_event_callback_call(imcontext->ctx, ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
    }
    //
}

// TIZEN_ONLY(20151221): Support input panel geometry
static void
text_input_input_panel_geometry(void                 *data EINA_UNUSED,
                                struct wl_text_input *text_input EINA_UNUSED,
                                uint32_t              x,
                                uint32_t              y,
                                uint32_t              w,
                                uint32_t              h)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)data;

    if (_keyboard_geometry.x != (int)x || _keyboard_geometry.y != (int)y ||
        _keyboard_geometry.w != (int)w || _keyboard_geometry.h != (int)h)
    {
        _keyboard_geometry.x = x;
        _keyboard_geometry.y = y;
        _keyboard_geometry.w = w;
        _keyboard_geometry.h = h;
        ecore_imf_context_input_panel_event_callback_call(imcontext->ctx, ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
    }
}
//

static void
text_input_language(void                 *data,
                    struct wl_text_input *text_input EINA_UNUSED,
                    uint32_t              serial EINA_UNUSED,
                    const char           *language)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)data;
    Eina_Bool changed = EINA_FALSE;

    if (!imcontext || !language) return;

    if (imcontext->language) {
        if (strcmp(imcontext->language, language) != 0) {
            changed = EINA_TRUE;
            free(imcontext->language);
        }
    }
    else {
        changed = EINA_TRUE;
    }

    if (changed) {
        imcontext->language = strdup(language);

        if (imcontext->ctx)
            ecore_imf_context_input_panel_event_callback_call(imcontext->ctx, ECORE_IMF_INPUT_PANEL_LANGUAGE_EVENT, 0);
    }
}

static void
text_input_text_direction(void                 *data EINA_UNUSED,
                          struct wl_text_input *text_input EINA_UNUSED,
                          uint32_t              serial EINA_UNUSED,
                          uint32_t              direction EINA_UNUSED)
{
}

// TIZEN_ONLY(20150918): Support to set the selection region
static void
text_input_selection_region(void                 *data,
                            struct wl_text_input *text_input EINA_UNUSED,
                            uint32_t              serial EINA_UNUSED,
                            int32_t               start,
                            int32_t               end)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)data;
    if (!imcontext || !imcontext->ctx) return;

    Ecore_IMF_Event_Selection ev;
    ev.ctx = imcontext->ctx;
    ev.start = start;
    ev.end = end;
    ecore_imf_context_event_callback_call(imcontext->ctx, ECORE_IMF_CALLBACK_SELECTION_SET, &ev);
}

static void
text_input_private_command(void                 *data,
                           struct wl_text_input *text_input EINA_UNUSED,
                           uint32_t              serial EINA_UNUSED,
                           const char           *command)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)data;
    if (!imcontext || !imcontext->ctx) return;

    const char *szConformantReset = "CONFORMANT_RESET";
    LOGD("Checking command : %s %s", command, szConformantReset);
    if (strncmp(command, szConformantReset, strlen(szConformantReset)) == 0) {
        LOGD("Resetting conformant area");

        Ecore_Wl_Window *window = imcontext->window;
        if (!window) return;

        ecore_wl_window_keyboard_geometry_set(window, 0, 0, 0, 0);

        Ecore_Wl_Event_Conformant_Change *ev;
        if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Conformant_Change)))) return;

        ev->win = ecore_wl_window_id_get(window);
        ev->part_type = 1;
        ev->state = 0;
        ecore_event_add(ECORE_WL_EVENT_CONFORMANT_CHANGE, ev, NULL, NULL);
    } else {
        ecore_imf_context_event_callback_call(imcontext->ctx, ECORE_IMF_CALLBACK_PRIVATE_COMMAND_SEND, (void *)command);
    }
}

static void
text_input_input_panel_data(void                 *data,
                            struct wl_text_input *text_input EINA_UNUSED,
                            uint32_t              serial EINA_UNUSED,
                            const char           *input_panel_data,
                            uint32_t              length)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)data;
    if (!imcontext || !imcontext->ctx) return;

    if (imcontext->input_panel_data)
        free (imcontext->input_panel_data);

    imcontext->input_panel_data = calloc (1, length);
    memcpy (imcontext->input_panel_data, input_panel_data, length);
    imcontext->input_panel_data_length = length;
}

static void
text_input_get_selection_text (void                 *data,
                               struct wl_text_input *text_input EINA_UNUSED,
                               int32_t              fd)
{
    char *selection = NULL;
    LOGD ("%d", fd);
    WaylandIMContext *imcontext = (WaylandIMContext *)data;
    if (!imcontext || !imcontext->ctx) {
        LOGD ("");
        close (fd);
        return;
    }

    ecore_imf_context_selection_get (imcontext->ctx, &selection);
    if (imcontext->text_input) {
        LOGD ("selection :%s", selection ? selection : "");
        if (selection) {
            char *_selection = selection;
            size_t len = strlen (selection);
            while (len) {
                ssize_t ret = write (fd, _selection, len);
                if (ret <= 0) {
                    if (errno == EINTR)
                        continue;
                    LOGW ("write pipe failed, errno: %d", errno);
                    break;
                }
                _selection += ret;
                len -= ret;
            }
        }
    }
    if (selection)
        free (selection);
    close (fd);
}

static void
text_input_get_surrounding_text (void                 *data,
                                 struct wl_text_input *text_input EINA_UNUSED,
                                 uint32_t              maxlen_before,
                                 uint32_t              maxlen_after,
                                 int32_t              fd)
{
    int cursor_pos;
    char *surrounding = NULL;
    LOGD("fd: %d maxlen_before: %d maxlen_after: %d", fd, maxlen_before, maxlen_after);
    WaylandIMContext *imcontext = (WaylandIMContext *)data;
    if (!imcontext || !imcontext->ctx) {
        LOGD("");
        close(fd);
        return;
    }

    /* cursor_pos is a byte index */
    if (ecore_imf_context_surrounding_get (imcontext->ctx, &surrounding, &cursor_pos)) {
        LOGD ("surrounding :%s, cursor: %d", surrounding ? surrounding : "", cursor_pos);
        if (imcontext->text_input) {
            Eina_Unicode *wide_surrounding = eina_unicode_utf8_to_unicode (surrounding, NULL);
            size_t wlen = eina_unicode_strlen (wide_surrounding);

            if (cursor_pos > (int)wlen || cursor_pos < 0)
                cursor_pos = 0;

            if (maxlen_before > cursor_pos)
                maxlen_before = 0;
            else
                maxlen_before = cursor_pos - maxlen_before;

            if (maxlen_after > wlen - cursor_pos)
                maxlen_after = wlen;
            else
                maxlen_after = cursor_pos + maxlen_after;

            char *req_surrounding = eina_unicode_unicode_to_utf8_range (wide_surrounding + maxlen_before, maxlen_after - maxlen_before, NULL);

            if (req_surrounding) {
                char *_surrounding = req_surrounding;
                size_t len = strlen(req_surrounding);
                while (len) {
                    ssize_t ret = write(fd, _surrounding, len);
                    if (ret <= 0) {
                        if (errno == EINTR)
                            continue;
                        LOGW ("write pipe failed, errno: %d", errno);
                        break;
                    }
                    _surrounding += ret;
                    len -= ret;
                }
            }

            if (req_surrounding)
                free (req_surrounding);

            if (wide_surrounding)
                free (wide_surrounding);
        }

        if (surrounding)
            free (surrounding);
    }
    close(fd);
}

static void
text_input_filter_key_event_done(void                 *data,
                                 struct wl_text_input *text_input EINA_UNUSED,
                                 uint32_t              serial,
                                 uint32_t              state)
{
    LOGD("serial: %d, state: %d", serial, state);
    WaylandIMContext *imcontext = (WaylandIMContext *)data;
    if (!imcontext) return;

    imcontext->last_key_event_filter.serial = serial;
    imcontext->last_key_event_filter.state = state;
}

static void
text_input_reset_done(void                 *data,
                      struct wl_text_input *text_input EINA_UNUSED,
                      uint32_t              serial)
{
    LOGD("serial: %d", serial);
    WaylandIMContext *imcontext = (WaylandIMContext *)data;
    if (!imcontext) return;

    imcontext->last_reset_serial = serial;
}

//

static const struct wl_text_input_listener text_input_listener =
{
    text_input_enter,
    text_input_leave,
    text_input_modifiers_map,
    text_input_input_panel_state,
    text_input_preedit_string,
    text_input_preedit_styling,
    text_input_preedit_cursor,
    text_input_commit_string,
    text_input_cursor_position,
    text_input_delete_surrounding_text,
    text_input_keysym,
    text_input_language,
    text_input_text_direction,
    // TIZEN_ONLY(20150918): Support to set the selection region
    text_input_selection_region,
    text_input_private_command,
    text_input_input_panel_geometry,
    text_input_input_panel_data,
    text_input_get_selection_text,
    text_input_get_surrounding_text,
    text_input_filter_key_event_done,
    text_input_reset_done
    //
};

#ifdef HAVE_VCONF
static void
keyboard_mode_changed_cb (keynode_t *key, void* data)
{
    int val;
    Ecore_IMF_Context *active_ctx = get_using_ctx ();

    if (vconf_get_bool (VCONFKEY_ISF_HW_KEYBOARD_INPUT_DETECTED, &val) != 0)
        return;

    if (active_ctx) {
        LOGD ("ctx : %p, input detect : %d\n", active_ctx, val);

        Ecore_IMF_Input_Panel_Keyboard_Mode input_mode = !val;
        ecore_imf_context_input_panel_event_callback_call (active_ctx, ECORE_IMF_INPUT_PANEL_KEYBOARD_MODE_EVENT, input_mode);

        if ((input_mode == ECORE_IMF_INPUT_PANEL_SW_KEYBOARD_MODE) && _focused_ctx && (active_ctx == _focused_ctx)) {
            if (ecore_imf_context_input_panel_enabled_get (active_ctx)) {
                ecore_imf_context_input_panel_show (active_ctx);
            }
        }
    }
}
#endif

EAPI void initialize ()
{
    register_key_handler ();

#ifdef HAVE_VCONF
    vconf_notify_key_changed (VCONFKEY_ISF_HW_KEYBOARD_INPUT_DETECTED, keyboard_mode_changed_cb, NULL);
#endif
}

EAPI void uninitialize ()
{
    unregister_key_handler ();

    _win_focus_out_handler_del ();
    _conformant_change_handler_del ();

#ifdef HAVE_VCONF
    vconf_ignore_key_changed (VCONFKEY_ISF_HW_KEYBOARD_INPUT_DETECTED, keyboard_mode_changed_cb);
#endif
}

EAPI void
wayland_im_context_add(Ecore_IMF_Context *ctx)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

    LOGD("context_add. ctx : %p", ctx);

    if (!imcontext) return;

    imcontext->ctx = ctx;
    imcontext->input_panel_layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;
    imcontext->keysym_list = NULL;
    imcontext->shift_mask = MOD_SHIFT_MASK;
    imcontext->control_mask = MOD_CONTROL_MASK;
    imcontext->alt_mask = MOD_ALT_MASK;

    imcontext->text_input =
        wl_text_input_manager_create_text_input(imcontext->text_input_manager);

    if (imcontext->text_input)
        wl_text_input_add_listener(imcontext->text_input,
                                   &text_input_listener, imcontext);
}

EAPI void
wayland_im_context_del (Ecore_IMF_Context *ctx)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

    Ecore_Event_Key *ev;
    LOGD ("context_del. ctx : %p, focused_ctx : %p, show_req_ctx : %p", ctx, _focused_ctx, _show_req_ctx);

    if (!imcontext) return;

    // TIZEN_ONLY(20150708): Support back key
    if (_hide_req_ctx == ctx && _hide_timer) {
        _input_panel_hide(ctx, EINA_TRUE);
        _input_panel_state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;
    }

    if (_focused_ctx == ctx)
        _focused_ctx = NULL;

    if (_show_req_ctx == ctx)
        _show_req_ctx = NULL;
    //

    if (imcontext->language) {
        free (imcontext->language);
        imcontext->language = NULL;
    }

    // TIZEN_ONLY(20150922): Support to set input panel data
    if (imcontext->imdata) {
        free (imcontext->imdata);
        imcontext->imdata = NULL;
        imcontext->imdata_size = 0;
    }

    if (imcontext->input_panel_data) {
        free (imcontext->input_panel_data);
        imcontext->input_panel_data = NULL;
        imcontext->input_panel_data_length = 0;
    }
    //

    if (imcontext->text_input)
        wl_text_input_destroy (imcontext->text_input);

    clear_preedit (imcontext);

    EINA_LIST_FREE(imcontext->keysym_list, ev) {
        free(ev);
    }
}

EAPI void
wayland_im_context_reset(Ecore_IMF_Context *ctx)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

    LOGD("ctx : %p", ctx);

    if (!imcontext) return;

    clear_preedit(imcontext);

    if (!imcontext->input) return;

    if (imcontext->text_input) {
        int serial = imcontext->serial++;
        double start_time = ecore_time_get();
        //send "reset_sync" to IME with serial
        wl_text_input_reset_sync(imcontext->text_input, serial);
        //wait for "reset_done" from IME according to serial
        while (ecore_time_get() - start_time < WAIT_FOR_FILTER_DONE_SECOND){
            wl_display_dispatch(ecore_wl_display_get());
            if (imcontext->last_reset_serial >= serial) {
                LOGD("reset_serial %d, serial %d", imcontext->last_reset_serial, serial);
                break;
            }
        }
    }

    update_state(imcontext);
}

EAPI void
wayland_im_context_focus_in(Ecore_IMF_Context *ctx)
{
    // TIZEN_ONLY(20150708): Support back key
    _focused_ctx = ctx;
    //

    set_focus(ctx);

    LOGD ("ctx : %p. on demand : %d\n", ctx, ecore_imf_context_input_panel_show_on_demand_get (ctx));

    if (ecore_imf_context_input_panel_enabled_get(ctx))
        if (!ecore_imf_context_input_panel_show_on_demand_get (ctx))
            show_input_panel(ctx);
        else
            LOGD ("ctx : %p input panel on demand mode : TRUE\n", ctx);
    else
        LOGD ("ctx : %p input panel enable : FALSE\n", ctx);
}

EAPI void
wayland_im_context_focus_out(Ecore_IMF_Context *ctx)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

    LOGD("ctx : %p", ctx);

    if (!imcontext || !imcontext->input) return;

    if (imcontext->text_input) {
        if (ecore_imf_context_input_panel_enabled_get(ctx))
            ecore_imf_context_input_panel_hide(ctx);

        wl_text_input_deactivate(imcontext->text_input,
                ecore_wl_input_seat_get(imcontext->input));
    }

    // TIZEN_ONLY(20150708): Support back key
    if (ctx == _focused_ctx)
        _focused_ctx = NULL;
    //

    imcontext->input = NULL;
}

EAPI void
wayland_im_context_preedit_string_get(Ecore_IMF_Context  *ctx,
                                      char              **str,
                                      int                *cursor_pos)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return;

    LOGD("pre-edit string requested (preedit: '%s')",
            imcontext->preedit_text ? imcontext->preedit_text : "");

    if (str)
        *str = strdup(imcontext->preedit_text ? imcontext->preedit_text : "");

    if (cursor_pos)
        *cursor_pos = imcontext->preedit_cursor;
}

EAPI void
wayland_im_context_preedit_string_with_attributes_get(Ecore_IMF_Context  *ctx,
                                                      char              **str,
                                                      Eina_List         **attrs,
                                                      int                *cursor_pos)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return;

    LOGD("pre-edit string with attributes requested (preedit: '%s')",
            imcontext->preedit_text ? imcontext->preedit_text : "");

    if (str)
        *str = strdup(imcontext->preedit_text ? imcontext->preedit_text : "");

    if (attrs) {
        Eina_List *l;
        Ecore_IMF_Preedit_Attr *a, *attr;

        EINA_LIST_FOREACH(imcontext->preedit_attrs, l, a) {
            attr = malloc(sizeof(*attr));
            attr = memcpy(attr, a, sizeof(*attr));
            *attrs = eina_list_append(*attrs, attr);
        }
    }

    if (cursor_pos)
        *cursor_pos = imcontext->preedit_cursor;
}

EAPI void
wayland_im_context_cursor_position_set (Ecore_IMF_Context *ctx,
                                       int                cursor_pos)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return;

    LOGD ("set cursor position (cursor: %d)", cursor_pos);
    if (imcontext->cursor_position != cursor_pos) {
        imcontext->cursor_position = cursor_pos;
        wl_text_input_set_cursor_position (imcontext->text_input, cursor_pos);
    }
}

EAPI void
wayland_im_context_use_preedit_set(Ecore_IMF_Context *ctx EINA_UNUSED,
                                   Eina_Bool          use_preedit EINA_UNUSED)
{
}

EAPI void
wayland_im_context_client_window_set(Ecore_IMF_Context *ctx,
                                     void              *window)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

    LOGD("client window set (window: %p)", window);

    if (imcontext && window)
        imcontext->window = ecore_wl_window_find((Ecore_Window)window);
}

EAPI void
wayland_im_context_client_canvas_set(Ecore_IMF_Context *ctx,
                                     void              *canvas)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

    LOGD("client canvas set (canvas: %p)", canvas);

    if (imcontext && canvas)
        imcontext->canvas = canvas;
}

EAPI void
wayland_im_context_show(Ecore_IMF_Context *ctx)
{
    LOGD("ctx : %p", ctx);

    show_input_panel(ctx);
}

EAPI void
wayland_im_context_hide(Ecore_IMF_Context *ctx)
{
    LOGD("ctx : %p", ctx);

    _input_panel_hide(ctx, EINA_FALSE);
}

EAPI Eina_Bool
wayland_im_context_filter_event(Ecore_IMF_Context    *ctx,
                                Ecore_IMF_Event_Type  type,
                                Ecore_IMF_Event      *event EINA_UNUSED)
{
    Eina_Bool ret = EINA_FALSE;

    if (type == ECORE_IMF_EVENT_MOUSE_UP) {
        if (ecore_imf_context_input_panel_enabled_get(ctx)) {
            LOGD ("[Mouse-up event] ctx : %p\n", ctx);
            if (ctx == _focused_ctx) {
                ecore_imf_context_input_panel_show(ctx);
            }
            else
                LOGE ("Can't show IME because there is no focus. ctx : %p\n", ctx);
        }
    } else if (type == ECORE_IMF_EVENT_KEY_UP || type == ECORE_IMF_EVENT_KEY_DOWN) {
        Ecore_Event_Key *ev =  (Ecore_Event_Key *)event;
        WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

        if (!_focused_ctx) {
            LOGD ("no focus\n");
            return EINA_FALSE;
        }

        if (!imcontext)
            return EINA_FALSE;

        int serial = imcontext->serial++;
        double start_time = ecore_time_get();

        uint32_t modifiers = 0;
        if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)
            modifiers |= imcontext->shift_mask;
        if (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)
            modifiers |= imcontext->control_mask;
        if (ev->modifiers & ECORE_EVENT_MODIFIER_ALT)
            modifiers |= imcontext->alt_mask;

        LOGD ("ev:modifiers=%d, modifiers=%d, shift_mask=%d, control_mask=%d, alt_mask=%d", ev->modifiers, modifiers, imcontext->shift_mask, imcontext->control_mask, imcontext->alt_mask);
        //Send key event to IME.
        wl_text_input_filter_key_event(imcontext->text_input, serial, ev->timestamp, ev->keyname,
                                       type == ECORE_IMF_EVENT_KEY_UP? WL_KEYBOARD_KEY_STATE_RELEASED : WL_KEYBOARD_KEY_STATE_PRESSED,
                                       modifiers);
        //Waiting for filter_key_event_done from IME.
        //This function should return IME filtering result with boolean type.
        while (ecore_time_get() - start_time < WAIT_FOR_FILTER_DONE_SECOND){
            wl_display_dispatch(ecore_wl_display_get());
            if (imcontext->last_key_event_filter.serial == serial) {
                ret = imcontext->last_key_event_filter.state;
                break;
            } else if (imcontext->last_key_event_filter.serial > serial)
                return EINA_FALSE;
        }

        LOGD ("elapsed : %.3f ms, serial (last, require) : (%d, %d)", (ecore_time_get() - start_time)*1000, imcontext->last_key_event_filter.serial, serial);
        //Deal with the next key event in list.
        if (eina_list_count (imcontext->keysym_list)) {
            Eina_List *n = eina_list_last(imcontext->keysym_list);
            ev = (Ecore_Event_Key *)eina_list_data_get(n);
            int type = (int)ev->data;

            ev->data = NULL;
            ecore_event_add(type, ev, NULL, NULL);
            imcontext->keysym_list = eina_list_remove_list(imcontext->keysym_list, n);
        }
    }

    return ret;
}

EAPI void
wayland_im_context_cursor_location_set(Ecore_IMF_Context *ctx, int x, int y, int width, int height)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return;

    if ((imcontext->cursor_location.x != x) ||
        (imcontext->cursor_location.y != y) ||
        (imcontext->cursor_location.width != width) ||
        (imcontext->cursor_location.height != height)) {
        imcontext->cursor_location.x = x;
        imcontext->cursor_location.y = y;
        imcontext->cursor_location.width = width;
        imcontext->cursor_location.height = height;

        if (_focused_ctx == ctx)
            send_cursor_location (imcontext);
    }
}

EAPI void wayland_im_context_autocapital_type_set(Ecore_IMF_Context *ctx,
                                                  Ecore_IMF_Autocapital_Type autocapital_type)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return;

    imcontext->content_hint &= ~(WL_TEXT_INPUT_CONTENT_HINT_AUTO_CAPITALIZATION |
                                 // TIZEN_ONLY(20160201): Add autocapitalization word
                                 WL_TEXT_INPUT_CONTENT_HINT_WORD_CAPITALIZATION |
                                 //
                                 WL_TEXT_INPUT_CONTENT_HINT_UPPERCASE |
                                 WL_TEXT_INPUT_CONTENT_HINT_LOWERCASE);

    if (autocapital_type == ECORE_IMF_AUTOCAPITAL_TYPE_SENTENCE)
        imcontext->content_hint |= WL_TEXT_INPUT_CONTENT_HINT_AUTO_CAPITALIZATION;
    // TIZEN_ONLY(20160201): Add autocapitalization word
    else if (autocapital_type == ECORE_IMF_AUTOCAPITAL_TYPE_WORD)
        imcontext->content_hint |= WL_TEXT_INPUT_CONTENT_HINT_WORD_CAPITALIZATION;
    //
    else if (autocapital_type == ECORE_IMF_AUTOCAPITAL_TYPE_ALLCHARACTER)
        imcontext->content_hint |= WL_TEXT_INPUT_CONTENT_HINT_UPPERCASE;
    else
        imcontext->content_hint |= WL_TEXT_INPUT_CONTENT_HINT_LOWERCASE;
}

EAPI void
wayland_im_context_input_panel_layout_set(Ecore_IMF_Context *ctx,
                                          Ecore_IMF_Input_Panel_Layout layout)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return;

    imcontext->input_panel_layout = layout;

    switch (layout) {
        case ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBER:
            imcontext->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_NUMBER;
            break;
        case ECORE_IMF_INPUT_PANEL_LAYOUT_EMAIL:
            imcontext->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_EMAIL;
            break;
        case ECORE_IMF_INPUT_PANEL_LAYOUT_URL:
            imcontext->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_URL;
            break;
        case ECORE_IMF_INPUT_PANEL_LAYOUT_PHONENUMBER:
            imcontext->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_PHONE;
            break;
        case ECORE_IMF_INPUT_PANEL_LAYOUT_IP:
            // TIZEN_ONLY(20150710): Support IP and emoticon layout
            imcontext->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_IP;
            //
            break;
        case ECORE_IMF_INPUT_PANEL_LAYOUT_MONTH:
            imcontext->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_DATE;
            break;
        case ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY:
            imcontext->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS;
            break;
        case ECORE_IMF_INPUT_PANEL_LAYOUT_HEX:
            imcontext->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_HEX;
            break;
        case ECORE_IMF_INPUT_PANEL_LAYOUT_TERMINAL:
            imcontext->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_TERMINAL;
            break;
        case ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD:
            imcontext->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_PASSWORD;
            break;
        case ECORE_IMF_INPUT_PANEL_LAYOUT_DATETIME:
            imcontext->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_DATETIME;
            break;
            // TIZEN_ONLY(20150710): Support IP and emoticon layout
        case ECORE_IMF_INPUT_PANEL_LAYOUT_EMOTICON:
            imcontext->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_EMOTICON;
            break;
            //
        default:
            imcontext->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_NORMAL;
            break;
    }
}

EAPI Ecore_IMF_Input_Panel_Layout
wayland_im_context_input_panel_layout_get(Ecore_IMF_Context *ctx)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;

    return imcontext->input_panel_layout;
}

EAPI void
wayland_im_context_input_mode_set(Ecore_IMF_Context *ctx,
                                  Ecore_IMF_Input_Mode input_mode)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return;

    if (input_mode & ECORE_IMF_INPUT_MODE_INVISIBLE)
        imcontext->content_hint |= WL_TEXT_INPUT_CONTENT_HINT_PASSWORD;
    else
        imcontext->content_hint &= ~WL_TEXT_INPUT_CONTENT_HINT_PASSWORD;
}

EAPI void
wayland_im_context_input_hint_set(Ecore_IMF_Context *ctx,
                                  Ecore_IMF_Input_Hints input_hints)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return;

    if (input_hints & ECORE_IMF_INPUT_HINT_AUTO_COMPLETE)
        imcontext->content_hint |= WL_TEXT_INPUT_CONTENT_HINT_AUTO_COMPLETION;
    else
        imcontext->content_hint &= ~WL_TEXT_INPUT_CONTENT_HINT_AUTO_COMPLETION;

    if (input_hints & ECORE_IMF_INPUT_HINT_SENSITIVE_DATA)
        imcontext->content_hint |= WL_TEXT_INPUT_CONTENT_HINT_SENSITIVE_DATA;
    else
        imcontext->content_hint &= ~WL_TEXT_INPUT_CONTENT_HINT_SENSITIVE_DATA;

    if (input_hints & ECORE_IMF_INPUT_HINT_MULTILINE)
        imcontext->content_hint |= WL_TEXT_INPUT_CONTENT_HINT_MULTILINE;
    else
        imcontext->content_hint &= ~WL_TEXT_INPUT_CONTENT_HINT_MULTILINE;
}

EAPI void
wayland_im_context_input_panel_language_set(Ecore_IMF_Context *ctx,
                                            Ecore_IMF_Input_Panel_Lang lang)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return;

    if (lang == ECORE_IMF_INPUT_PANEL_LANG_ALPHABET)
        imcontext->content_hint |= WL_TEXT_INPUT_CONTENT_HINT_LATIN;
    else
        imcontext->content_hint &= ~WL_TEXT_INPUT_CONTENT_HINT_LATIN;
}

// TIZEN_ONLY(20150708): Support input_panel_state_get
EAPI Ecore_IMF_Input_Panel_State
wayland_im_context_input_panel_state_get(Ecore_IMF_Context *ctx EINA_UNUSED)
{
    return _input_panel_state;
}

EAPI void
wayland_im_context_input_panel_return_key_type_set(Ecore_IMF_Context *ctx,
                                                   Ecore_IMF_Input_Panel_Return_Key_Type return_key_type)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return;

    imcontext->return_key_type = return_key_type;

    if (imcontext->input && imcontext->text_input)
        wl_text_input_set_return_key_type(imcontext->text_input,
                imcontext->return_key_type);
}

EAPI void
wayland_im_context_input_panel_return_key_disabled_set(Ecore_IMF_Context *ctx,
                                                       Eina_Bool disabled)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return;

    imcontext->return_key_disabled = disabled;

    if (imcontext->input && imcontext->text_input)
        wl_text_input_set_return_key_disabled(imcontext->text_input,
                imcontext->return_key_disabled);
}
//

EAPI void
wayland_im_context_input_panel_language_locale_get(Ecore_IMF_Context *ctx,
                                                   char **locale)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return;

    if (locale)
        *locale = strdup(imcontext->language ? imcontext->language : "");
}

EAPI void
wayland_im_context_prediction_allow_set(Ecore_IMF_Context *ctx,
                                        Eina_Bool prediction)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return;

    if (prediction)
        imcontext->content_hint |= WL_TEXT_INPUT_CONTENT_HINT_AUTO_COMPLETION;
    else
        imcontext->content_hint &= ~WL_TEXT_INPUT_CONTENT_HINT_AUTO_COMPLETION;
}

// TIZEN_ONLY(20151221): Support input panel geometry
EAPI void
wayland_im_context_input_panel_geometry_get(Ecore_IMF_Context *ctx EINA_UNUSED,
                                            int *x, int *y, int *w, int *h)
{
    if (x)
        *x = _keyboard_geometry.x;
    if (y)
        *y = _keyboard_geometry.y;
    if (w)
        *w = _keyboard_geometry.w;
    if (h)
        *h = _keyboard_geometry.h;
}

EAPI void
wayland_im_context_input_panel_imdata_set(Ecore_IMF_Context *ctx, const void *data, int length)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return;

    if (imcontext->imdata)
        free(imcontext->imdata);

    imcontext->imdata = calloc(1, length);
    memcpy(imcontext->imdata, data, length);
    imcontext->imdata_size = length;

    if (imcontext->input && (imcontext->imdata_size > 0))
        wl_text_input_set_input_panel_data(imcontext->text_input, (const char *)imcontext->imdata, imcontext->imdata_size);
}

EAPI void
wayland_im_context_input_panel_imdata_get(Ecore_IMF_Context *ctx, void *data, int *length)
{
    if (!ctx) return;
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);

    if (imcontext && imcontext->input_panel_data && (imcontext->input_panel_data_length > 0)) {
        if (data)
            memcpy(data, imcontext->input_panel_data, imcontext->input_panel_data_length);

        if (length)
            *length = imcontext->input_panel_data_length;
    }
    else
        if (length)
            *length = 0;
}
//

// TIZEN_ONLY(20160218): Support BiDi direction
EAPI void
wayland_im_context_bidi_direction_set(Ecore_IMF_Context *ctx, Ecore_IMF_BiDi_Direction bidi_direction)
{
    WaylandIMContext *imcontext = (WaylandIMContext *)ecore_imf_context_data_get(ctx);
    if (!imcontext) return;

    imcontext->bidi_direction = bidi_direction;

    if (imcontext->text_input)
        wl_text_input_bidi_direction(imcontext->text_input, imcontext->bidi_direction);
}
//

WaylandIMContext *wayland_im_context_new (struct wl_text_input_manager *text_input_manager)
{
    WaylandIMContext *context = calloc(1, sizeof(WaylandIMContext));

    LOGD("new context created");
    context->text_input_manager = text_input_manager;

    return context;
}
