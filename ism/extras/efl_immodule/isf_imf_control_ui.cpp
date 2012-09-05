/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jihoon Kim <jihoon48.kim@samsung.com>, Haifeng Deng <haifeng.deng@samsung.com>
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

#define Uses_SCIM_TRANSACTION


/* IM control UI part */
#include <Ecore_X.h>
#include <Ecore_Evas.h>
#include <Ecore_IMF.h>
#include <glib.h>
#include "scim.h"
#include "isf_imf_control_ui.h"
#include "isf_imf_context.h"
#include "isf_imf_control.h"
#include "ise_context.h"

using namespace scim;

#define IMFCONTROLUIDBG(str...)
#define IMFCONTROLUIERR(str...) printf(str)

typedef struct {
    void (*func)(void *data, Ecore_IMF_Context *ctx, int value);
    void *data;
    Ecore_IMF_Input_Panel_Event type;
    Ecore_IMF_Context *imf_context;
} EventCallbackNode;


/* IM control related variables */
static Ise_Context        iseContext;
static bool               IfInitContext     = false;
static Eina_List         *EventCallbackList = NULL;
static Ecore_IMF_Context *show_req_ic       = NULL;
static Ecore_IMF_Context *hide_req_ic       = NULL;
static Ecore_Event_Handler *_prop_change_handler = NULL;
static Ecore_X_Atom       prop_x_ext_keyboard_exist = 0;
static Ecore_X_Window     _rootwin;
static unsigned int       hw_kbd_num = 0;
static Ecore_Timer       *hide_timer = NULL;
static Ecore_IMF_Input_Panel_State input_panel_state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;

Ecore_IMF_Context        *input_panel_ctx = NULL;

static Eina_Bool _prop_change (void *data, int ev_type, void *ev)
{
    Ecore_X_Event_Window_Property *event = (Ecore_X_Event_Window_Property *)ev;
    unsigned int val = 0;

    if (event->win != _rootwin) return ECORE_CALLBACK_PASS_ON;
    if (event->atom != prop_x_ext_keyboard_exist) return ECORE_CALLBACK_PASS_ON;

    if (!ecore_x_window_prop_card32_get (event->win, prop_x_ext_keyboard_exist, &val, 1) > 0)
        return ECORE_CALLBACK_PASS_ON;

    if (val != 0) {
        if (show_req_ic)
            isf_imf_context_input_panel_hide (show_req_ic);
    }

    hw_kbd_num = val;
    LOGD ("The number of connected H/W keyboard : %d\n", hw_kbd_num);

    return ECORE_CALLBACK_PASS_ON;
}

static Ecore_X_Window _client_window_id_get (Ecore_IMF_Context *ctx)
{
    Ecore_X_Window xid = 0;
    Evas *evas = NULL;
    Ecore_Evas *ee = NULL;

    evas = (Evas *)ecore_imf_context_client_canvas_get (ctx);

    if (evas) {
        ee = ecore_evas_ecore_evas_get (evas);
        if (ee)
            xid = (Ecore_X_Window)ecore_evas_window_get (ee);
    } else {
        xid = (Ecore_X_Window)ecore_imf_context_client_window_get (ctx);
    }

    return xid;
}

static void _save_current_xid (Ecore_IMF_Context *ctx)
{
    Ecore_X_Window xid = 0, rootwin_xid = 0;

    xid = _client_window_id_get (ctx);

    if (xid == 0)
        rootwin_xid = ecore_x_window_root_first_get ();
    else
        rootwin_xid = ecore_x_window_root_get (xid);

    Ecore_X_Atom isf_active_window_atom = ecore_x_atom_get ("_ISF_ACTIVE_WINDOW");
    ecore_x_window_prop_property_set (rootwin_xid, isf_active_window_atom, ((Ecore_X_Atom) 33), 32, &xid, 1);
    ecore_x_flush ();
    ecore_x_sync ();
}

static void _event_callback_call (Ecore_IMF_Input_Panel_Event type, int value)
{
    void *list_data = NULL;
    EventCallbackNode *fn = NULL;
    Eina_List *l = NULL;
    Ecore_IMF_Context *using_ic = show_req_ic;

    if (type == ECORE_IMF_INPUT_PANEL_STATE_EVENT &&
        value == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
        if (hide_req_ic) {
            using_ic = hide_req_ic;
            hide_req_ic = NULL;
        }
    }

    EINA_LIST_FOREACH(EventCallbackList, l, list_data) {
        fn = (EventCallbackNode *)list_data;

        if ((fn) && (fn->imf_context == using_ic) &&
            (fn->type == type) && (fn->func)) {
            if (type == ECORE_IMF_INPUT_PANEL_STATE_EVENT) {
                switch (value)
                {
                    case ECORE_IMF_INPUT_PANEL_STATE_HIDE:
                        LOGD ("[input panel has been hidden] ctx : %p\n", fn->imf_context);
                        break;
                    case ECORE_IMF_INPUT_PANEL_STATE_SHOW:
                        LOGD ("[input panel has been shown] ctx : %p\n", fn->imf_context);
                        break;
                    case ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW:
                        LOGD ("[input panel will be shown] ctx : %p\n", fn->imf_context);
                        break;
                }
            }
            fn->func (fn->data, fn->imf_context, value);
        }

        IMFCONTROLUIDBG("\tFunc : %p\tType : %d\n", fn->func, fn->type);
    }
}

static void _isf_imf_context_init (void)
{
    IMFCONTROLUIDBG("debug start --%s\n", __FUNCTION__);
    iseContext.language            = ECORE_IMF_INPUT_PANEL_LANG_AUTOMATIC;
    iseContext.return_key_type     = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT;
    iseContext.return_key_disabled = FALSE;
    iseContext.prediction_allow    = TRUE;

    if (!IfInitContext) {
        IfInitContext = true;
    }

    IMFCONTROLUIDBG("debug end\n");
}

static Eina_Bool _hide_timer_handler (void *data)
{
    LOGD ("[request to hide input panel] ctx : %p\n", data);
    _isf_imf_context_input_panel_hide ();

    hide_timer = NULL;
    return ECORE_CALLBACK_CANCEL;
}

static void _input_panel_hide_timer_start(void *data)
{
    if (!hide_timer)
        hide_timer = ecore_timer_add (0.05, _hide_timer_handler, data);
}

static void _input_panel_hide (Ecore_IMF_Context *ctx, Eina_Bool instant)
{
    IMFCONTROLUIDBG("[%s]\n", __func__);

    if (IfInitContext == false) {
        _isf_imf_context_init ();
    }

    if (input_panel_state == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
        hide_req_ic = ctx;
    }

    if (instant) {
        if (hide_timer) {
            ecore_timer_del (hide_timer);
            hide_timer = NULL;
        }

        LOGD ("[request to hide input panel] ctx : %p\n", ctx);
        _isf_imf_context_input_panel_hide ();
    } else {
        _input_panel_hide_timer_start (ctx);
    }
}

static Eina_Bool _compare_context (Ecore_IMF_Context *ctx1, Ecore_IMF_Context *ctx2)
{
    if (!ctx1 || !ctx2) return EINA_FALSE;

    if ((ecore_imf_context_autocapital_type_get (ctx1) == ecore_imf_context_autocapital_type_get (ctx2)) &&
        (ecore_imf_context_input_panel_layout_get (ctx1) == ecore_imf_context_input_panel_layout_get (ctx2)) &&
        (ecore_imf_context_input_panel_language_get (ctx1) == ecore_imf_context_input_panel_language_get (ctx2)) &&
        (ecore_imf_context_input_panel_return_key_type_get (ctx1) == ecore_imf_context_input_panel_return_key_type_get (ctx2)) &&
        (ecore_imf_context_input_panel_return_key_disabled_get (ctx1) == ecore_imf_context_input_panel_return_key_disabled_get (ctx2)) &&
        (ecore_imf_context_input_panel_caps_lock_mode_get (ctx1) == ecore_imf_context_input_panel_caps_lock_mode_get (ctx2)))
        return EINA_TRUE;

    return EINA_FALSE;
}

EAPI void isf_imf_context_control_panel_show (Ecore_IMF_Context *ctx)
{
    IMFCONTROLUIDBG("[%s]\n", __FUNCTION__);

    if (IfInitContext == false) {
        _isf_imf_context_init ();
    }
    _isf_imf_context_control_panel_show ();
}

EAPI void isf_imf_context_control_panel_hide (Ecore_IMF_Context *ctx)
{
    IMFCONTROLUIDBG("[%s]\n", __FUNCTION__);

    if (IfInitContext == false) {
        _isf_imf_context_init ();
    }
    _isf_imf_context_control_panel_hide ();
}

EAPI void isf_imf_input_panel_init (void)
{
    IMFCONTROLUIDBG("[%s]\n", __FUNCTION__);

    if (_prop_change_handler) return;

    _rootwin = ecore_x_window_root_first_get ();
    ecore_x_event_mask_set (_rootwin, ECORE_X_EVENT_MASK_WINDOW_PROPERTY);

    _prop_change_handler = ecore_event_handler_add (ECORE_X_EVENT_WINDOW_PROPERTY, _prop_change, NULL);

    if (!prop_x_ext_keyboard_exist)
        prop_x_ext_keyboard_exist = ecore_x_atom_get (PROP_X_EXT_KEYBOARD_EXIST);

    if (!ecore_x_window_prop_card32_get (_rootwin, prop_x_ext_keyboard_exist, &hw_kbd_num, 1)) {
        printf ("Error! cannot get hw_kbd_num\n");
        return;
    }

    LOGD ("The number of connected H/W keyboard : %d\n", hw_kbd_num);
}

EAPI void isf_imf_input_panel_shutdown (void)
{
    IMFCONTROLUIDBG("[%s]\n", __FUNCTION__);

    if (_prop_change_handler) {
        ecore_event_handler_del (_prop_change_handler);
        _prop_change_handler = NULL;
    }

    if (hide_timer) {
        if (input_panel_state != ECORE_IMF_INPUT_PANEL_STATE_HIDE)
            _isf_imf_context_input_panel_hide ();
        ecore_timer_del (hide_timer);
        hide_timer = NULL;
    }
    _isf_imf_control_finalize ();
}

EAPI void isf_imf_context_input_panel_show (Ecore_IMF_Context* ctx)
{
    int length = -1;
    void *packet = NULL;
    char imdata[1024] = {0};

    input_panel_ctx = ctx;

    IMFCONTROLUIDBG("debug start --%s\n", __FUNCTION__);

    if (IfInitContext == false) {
        _isf_imf_context_init ();
    }

    /* set password mode */
    iseContext.password_mode = !!(ecore_imf_context_input_mode_get (ctx) & ECORE_IMF_INPUT_MODE_INVISIBLE);

    /* set layout in ise context info */
    iseContext.layout = ecore_imf_context_input_panel_layout_get (ctx);

    /* set prediction allow */
    iseContext.prediction_allow = ecore_imf_context_prediction_allow_get (ctx);

    if (iseContext.layout == ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD)
        iseContext.password_mode = EINA_TRUE;

    if (iseContext.password_mode)
        iseContext.prediction_allow = EINA_FALSE;

    if (iseContext.layout == ECORE_IMF_INPUT_PANEL_LAYOUT_EMAIL ||
        iseContext.layout == ECORE_IMF_INPUT_PANEL_LAYOUT_URL ||
        iseContext.layout == ECORE_IMF_INPUT_PANEL_LAYOUT_TERMINAL)
        iseContext.prediction_allow = EINA_FALSE;

    isf_imf_context_prediction_allow_set (ctx, iseContext.prediction_allow);

    /* Set the current XID of the active window into the root window property */
    _save_current_xid (ctx);

    if (hw_kbd_num != 0) {
        printf ("H/W keyboard is existed.\n");
        return;
    }

    if (hide_timer) {
        ecore_timer_del(hide_timer);
        hide_timer = NULL;
        hide_req_ic = NULL;
    }

    if ((show_req_ic == ctx) &&
        (_compare_context (show_req_ic, ctx) == EINA_TRUE) &&
        (input_panel_state == ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW)) {
        return;
    }

    show_req_ic = ctx;

    LOGD ("===============================================================\n");
    LOGD ("[request to show input panel] ctx : %p\n", ctx);
    LOGD (" - language : %d\n", iseContext.language);
    LOGD (" - layout : %d\n", iseContext.layout);

    /* set return key type */
    iseContext.return_key_type = ecore_imf_context_input_panel_return_key_type_get (ctx);
    LOGD (" - return_key_type : %d\n", iseContext.return_key_type);

    /* set return key disabled */
    iseContext.return_key_disabled = ecore_imf_context_input_panel_return_key_disabled_get (ctx);
    LOGD (" - return_key_disabled : %d\n", iseContext.return_key_disabled);

    /* set caps mode */
    iseContext.caps_mode = caps_mode_check (ctx, EINA_TRUE, EINA_FALSE);
    LOGD (" - caps mode : %d\n", iseContext.caps_mode);

    /* set X Client window ID */
    iseContext.client_window = _client_window_id_get (ctx);
    LOGD (" - client_window : %#x\n", iseContext.client_window);

    /* set the size of imdata */
    ecore_imf_context_input_panel_imdata_get (ctx, (void *)imdata, &iseContext.imdata_size);

    LOGD (" - password mode : %d\n", iseContext.password_mode);
    LOGD (" - prediction_allow : %d\n", iseContext.prediction_allow);

    /* set the cursor position of the editable widget */
    ecore_imf_context_surrounding_get (ctx, NULL, &iseContext.cursor_pos);
    LOGD (" - cursor position : %d\n", iseContext.cursor_pos);

    /* calculate packet size */
    length = sizeof (iseContext);
    length += iseContext.imdata_size;

    /* create packet */
    packet = calloc (1, length);
    if (!packet)
        return;

    memcpy (packet, (void *)&iseContext, sizeof (iseContext));

    memcpy ((void *)((unsigned int)packet + sizeof (iseContext)), (void *)imdata, iseContext.imdata_size);

    if (input_panel_state == ECORE_IMF_INPUT_PANEL_STATE_HIDE)
        input_panel_state = ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW;

    _isf_imf_context_input_panel_show (packet ,length);
    free (packet);

    caps_mode_check(ctx, EINA_TRUE, EINA_TRUE);
    LOGD ("===============================================================\n");
}

EAPI void isf_imf_context_input_panel_hide (Ecore_IMF_Context *ctx)
{
    LOGD ("[input panel hide is called] ctx : %p\n", ctx);

    _input_panel_hide (ctx, EINA_FALSE);
}

EAPI void isf_imf_context_input_panel_instant_hide (Ecore_IMF_Context *ctx)
{
    IMFCONTROLUIDBG("[%s]\n", __func__);

    _input_panel_hide (ctx, EINA_TRUE);
}

EAPI void isf_imf_context_input_panel_language_set (Ecore_IMF_Context *ctx, Ecore_IMF_Input_Panel_Lang language)
{
    IMFCONTROLUIDBG("[%s] language : %d\n", __func__, language);

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();
    iseContext.language = language;

    if (context_scim == get_focused_ic ())
        _isf_imf_context_input_panel_language_set (language);
}

EAPI Ecore_IMF_Input_Panel_Lang isf_imf_context_input_panel_language_get (Ecore_IMF_Context *ctx)
{
    IMFCONTROLUIDBG("[%s] language : %d\n", __func__, iseContext.language);
    if (!IfInitContext) _isf_imf_context_init();
    return iseContext.language;
}

EAPI void isf_imf_context_input_panel_language_locale_get (Ecore_IMF_Context *ctx, char **locale)
{
    if (!IfInitContext)
        _isf_imf_context_init ();

    if (locale)
        _isf_imf_context_input_panel_language_locale_get (locale);
}

EAPI void isf_imf_context_input_panel_caps_mode_set (Ecore_IMF_Context *ctx, unsigned int mode)
{
    IMFCONTROLUIDBG("[%s] shift mode : %d\n", __func__, mode);

    if (!IfInitContext)
        _isf_imf_context_init ();
    _isf_imf_context_input_panel_caps_mode_set (mode);
}

EAPI void isf_imf_context_input_panel_caps_lock_mode_set (Ecore_IMF_Context *ctx, Eina_Bool mode)
{
    IMFCONTROLUIDBG("[%s] caps lock mode : %d\n", __func__, mode);

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();

    if (context_scim == get_focused_ic ())
        caps_mode_check(ctx, EINA_TRUE, EINA_TRUE);
}

/**
 * Set up an ISE specific data
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[in] data pointer of data to sets up to ISE
 * @param[in] length length of data
 */
EAPI void isf_imf_context_input_panel_imdata_set (Ecore_IMF_Context *ctx, const void* data, int length)
{
    IMFCONTROLUIDBG("[%s] data : %s, len : %d\n", __func__, data, length);

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (context_scim == get_focused_ic ())
        _isf_imf_context_input_panel_imdata_set (data, length);
}

/**
 * Get the ISE specific data from ISE
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[out] data pointer of data to return
 * @param[out] length length of data
 */
EAPI void isf_imf_context_input_panel_imdata_get (Ecore_IMF_Context *ctx, void* data, int* length)
{
    if (!IfInitContext)
        _isf_imf_context_init ();
    _isf_imf_context_input_panel_imdata_get (data, length);
    IMFCONTROLUIDBG("[%s] imdata : %s, len : %d\n", __func__, data, *length);
}

/**
 * Get ISE's position and size, in screen coodinates of the ISE rectangle not the client area,
 * the represents the size and location of the ISE
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[out] x the x position of ISE window
 * @param[out] y the y position of ISE window
 * @param[out] w the width of ISE window
 * @param[out] h the height of ISE window
 */
EAPI void isf_imf_context_input_panel_geometry_get (Ecore_IMF_Context *ctx, int *x, int *y, int *w, int *h)
{
    if (!IfInitContext)
        _isf_imf_context_init ();
    _isf_imf_context_input_panel_geometry_get (x, y, w, h);

    LOGD ("[input_panel_geometry_get] ctx : %p, x : %d, y : %d, w : %d, h : %d\n", ctx, *x, *y, *w, *h);
}

/**
 * Set type of return key.
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[in] type the type of return key
 */
EAPI void isf_imf_context_input_panel_return_key_type_set (Ecore_IMF_Context *ctx, Ecore_IMF_Input_Panel_Return_Key_Type type)
{
    IMFCONTROLUIDBG("[%s] value : %d\n", __func__, type);

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();

    if (context_scim == get_focused_ic ()) {
        _isf_imf_context_input_panel_return_key_type_set (type);
    }
}

/**
 * Get type of return key.
 *
 * @param[in] ctx a #Ecore_IMF_Context
 *
 * @return the type of return key.
 */
EAPI Ecore_IMF_Input_Panel_Return_Key_Type isf_imf_context_input_panel_return_key_type_get (Ecore_IMF_Context *ctx)
{
    IMFCONTROLUIDBG("[%s]\n", __func__);

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();

    Ecore_IMF_Input_Panel_Return_Key_Type type = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT;
    if (context_scim == get_focused_ic ())
        _isf_imf_context_input_panel_return_key_type_get (type);

    return type;
}

/**
 * Make return key to be disabled in active ISE keyboard layout for own's application.
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[in] disabled the state
 */
EAPI void isf_imf_context_input_panel_return_key_disabled_set (Ecore_IMF_Context *ctx, Eina_Bool disabled)
{
    IMFCONTROLUIDBG("[%s] value : %d\n", __func__, disabled);

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();

    if (context_scim == get_focused_ic ())
        _isf_imf_context_input_panel_return_key_disabled_set (disabled);
}

/**
 * Get the disabled status of return key.
 *
 * @param[in] ctx a #Ecore_IMF_Context
 *
 * @return the disable state of return key.
 */
EAPI Eina_Bool isf_imf_context_input_panel_return_key_disabled_get (Ecore_IMF_Context *ctx)
{
    IMFCONTROLUIDBG("[%s]\n", __func__);

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();

    Eina_Bool disabled = EINA_FALSE;
    if (context_scim == get_focused_ic ())
        _isf_imf_context_input_panel_return_key_disabled_get (disabled);

    return disabled;
}

/**
 * Sets up the layout infomation of active ISE
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[in] layout sets a layout ID to be shown. The layout ID will define by the configuration of selected ISE.
 */
EAPI void
isf_imf_context_input_panel_layout_set (Ecore_IMF_Context *ctx, Ecore_IMF_Input_Panel_Layout layout)
{
    IMFCONTROLUIDBG("[%s] layout : %d\n", __func__, layout);

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();

    if (context_scim == get_focused_ic ())
        _isf_imf_context_input_panel_layout_set (layout);
}

/**
* Get current ISE layout
*
* @param[in] ctx a #Ecore_IMF_Context
*
* @return the layout of current ISE.
*/
EAPI Ecore_IMF_Input_Panel_Layout isf_imf_context_input_panel_layout_get (Ecore_IMF_Context *ctx)
{
    Ecore_IMF_Input_Panel_Layout layout;
    if (!IfInitContext)
        _isf_imf_context_init ();
    _isf_imf_context_input_panel_layout_get (&layout);

    IMFCONTROLUIDBG("[%s] layout : %d\n", __func__, layout);

    return layout;
}

/**
 * Get current ISE state
 *
 * @param[in] ctx a #Ecore_IMF_Context
 *
 * @return the state of current ISE.
 */
EAPI Ecore_IMF_Input_Panel_State isf_imf_context_input_panel_state_get (Ecore_IMF_Context *ctx)
{
    IMFCONTROLUIDBG("[%s] state : %d\n", __func__, input_panel_state);

    if (!IfInitContext)
        _isf_imf_context_init ();
    return input_panel_state;
}

EAPI void isf_imf_context_input_panel_event_callback_add (Ecore_IMF_Context *ctx,
                                                          Ecore_IMF_Input_Panel_Event type,
                                                          void (*func) (void *data, Ecore_IMF_Context *ctx, int value),
                                                          void *data)
{
    EventCallbackNode *fn = NULL;

    fn = (EventCallbackNode *)calloc (1, sizeof (EventCallbackNode));
    if (!fn)
        return;

    LOGD ("[input_panel_event_callback_add] ctx : %p, type : %d, func : %p\n", ctx, type, func);

    fn->func = func;
    fn->data = data;
    fn->type = type;
    fn->imf_context = ctx;

    EventCallbackList = eina_list_append (EventCallbackList, fn);
}

EAPI void isf_imf_context_input_panel_event_callback_del (Ecore_IMF_Context *ctx,
                                                          Ecore_IMF_Input_Panel_Event type,
                                                          void (*func) (void *data, Ecore_IMF_Context *ctx, int value))
{
    Eina_List *l = NULL;
    EventCallbackNode *fn = NULL;

    IMFCONTROLUIDBG("[%s]\n", __func__);
    LOGD ("[input_panel_event_callback_del] ctx : %p, type : %d, func : %p\n", ctx, type, func);

    for (l = EventCallbackList; l;) {
        fn = (EventCallbackNode *)l->data;

        if ((fn) && (fn->func == func) && (fn->type == type) && (fn->imf_context == ctx)) {
            EventCallbackList = eina_list_remove (EventCallbackList, fn);
            free (fn);
            break;
        }
        l = l->next;
    }
}

EAPI void isf_imf_context_input_panel_event_callback_clear (Ecore_IMF_Context *ctx)
{
    Eina_List *l;
    EventCallbackNode *fn;

    LOGD ("[input_panel_event_callback_clear] ctx : %p\n", ctx);

    for (l = EventCallbackList; l;) {
        fn = (EventCallbackNode *)l->data;

        if ((fn) && (fn->imf_context == ctx)) {
            EventCallbackList = eina_list_remove (EventCallbackList, fn);
            free (fn);
        }
        l = l->next;
    }
}

EAPI void input_panel_event_callback_call (Ecore_IMF_Input_Panel_Event type, int value)
{
    _event_callback_call (type, value);
}

/**
 * Get candidate window position and size, in screen coodinates of the candidate rectangle not the client area,
 * the represents the size and location of the candidate window
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[out] x the x position of candidate window
 * @param[out] y the y position of candidate window
 * @param[out] w the width of candidate window
 * @param[out] h the height of candidate window
 */
EAPI void isf_imf_context_candidate_window_geometry_get (Ecore_IMF_Context *ctx, int *x, int *y, int *w, int *h)
{
    if (!IfInitContext)
        _isf_imf_context_init ();
    _isf_imf_context_candidate_window_geometry_get (x, y, w, h);

    LOGD ("[candidate_window_geometry_get] ctx : %p, x : %d, y : %d, w : %d, h : %d\n", ctx, *x, *y, *w, *h);
}

/**
 * process command message, ISM_TRANS_CMD_ISE_PANEL_SHOWED of ecore_ise_process_event()
 */
static bool _process_ise_panel_showed (void)
{
    /* When the size of ISE gets changed, STATE_SHOW is be delivered again to letting applications know the change.
       Later, an event type for notifying the size change of ISE needs to be added instead. */
    input_panel_state = ECORE_IMF_INPUT_PANEL_STATE_SHOW;

    /* Notify that ISE status has changed */
    _event_callback_call (ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_SHOW);

    return true;
}

/**
 * process command message, ISM_TRANS_CMD_ISE_PANEL_HIDED of ecore_ise_process_event()
 */
static bool _process_ise_panel_hided (void)
{
    if (input_panel_state == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
        IMFCONTROLUIDBG("ISE is already hided (_process_ise_panel_hided)\n");
        return false;
    }

    input_panel_state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;

    /* Notify that ISE status has changed */
    _event_callback_call (ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_HIDE);

    return true;
}

/**
 * process command message, ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT of gtk_ise_process_event()
 */
static bool _process_update_input_context (Transaction &trans)
{
    uint32 type;
    uint32 value;

    if (!(trans.get_data (type) && trans.get_data (value)))
        return false;

    IMFCONTROLUIDBG("[%s] receive input context: [%d:%d]\n", __FUNCTION__, type, value);

    if (type == (uint32)ECORE_IMF_INPUT_PANEL_STATE_EVENT) {
        switch(value)
        {
            case ECORE_IMF_INPUT_PANEL_STATE_HIDE:
                _process_ise_panel_hided ();
                return true;
            case ECORE_IMF_INPUT_PANEL_STATE_SHOW:
                _process_ise_panel_showed ();
                return true;
            case ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW:
                input_panel_state = ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW;
                break;
            default:
                break;
        }
    }

    _event_callback_call ((Ecore_IMF_Input_Panel_Event)type, (int)value);

    return true;
}

/**
 * process ISE data of command message with ISF
 *
 * @param[in] trans packet data to be processed
 * @param[in] cmd command ID that defines with ISF
 */
void ecore_ise_process_data (Transaction &trans, int cmd)
{
    switch (cmd) {
    case ISM_TRANS_CMD_ISE_PANEL_SHOWED : {
        IMFCONTROLUIDBG ("cmd is ISM_TRANS_CMD_ISE_PANEL_SHOWED\n");
        _process_ise_panel_showed ();
        break;
    }
    case ISM_TRANS_CMD_ISE_PANEL_HIDED : {
        IMFCONTROLUIDBG ("cmd is ISM_TRANS_CMD_ISE_PANEL_HIDED\n");
        _process_ise_panel_hided ();
        break;
    }
    case ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT : {
        IMFCONTROLUIDBG ("cmd is ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT\n");
        _process_update_input_context (trans);
        break;
    }
    case ISM_TRANS_CMD_UPDATE_ISF_CANDIDATE_PANEL : {
        IMFCONTROLUIDBG ("cmd is ISM_TRANS_CMD_UPDATE_ISF_CANDIDATE_PANEL\n");
        _process_update_input_context (trans);
        break;
    }
    default :
        IMFCONTROLUIDBG("unknown command");
        break;
    }
}

