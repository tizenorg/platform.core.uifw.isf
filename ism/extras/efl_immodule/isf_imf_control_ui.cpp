/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
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

    return ECORE_CALLBACK_PASS_ON;
}

static void _save_current_xid (Ecore_IMF_Context *ctx)
{
    Ecore_X_Window xid = 0, rootwin_xid = 0;
    Ecore_Evas *ee = NULL;
    Evas *evas = NULL;

    evas = (Evas *)ecore_imf_context_client_canvas_get(ctx);

    if (evas) {
        ee = ecore_evas_ecore_evas_get (evas);
        if (ee)
            xid = (Ecore_X_Window)ecore_evas_window_get (ee);
    } else {
        xid = (Ecore_X_Window)ecore_imf_context_client_window_get (ctx);
    }

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
            (fn->type == type) && (fn->func))
            fn->func (fn->data, fn->imf_context, value);

        IMFCONTROLUIDBG("\tFunc : %p\tType : %d\n", fn->func, fn->type);
    }
}

static void _isf_imf_context_init (void)
{
    IMFCONTROLUIDBG("debug start --%s\n", __FUNCTION__);
    memset (iseContext.name, '\0', sizeof (iseContext.name));
    iseContext.IfAlwaysShow = FALSE;
    iseContext.IfFullStyle  = FALSE;
    iseContext.state        = ECORE_IMF_INPUT_PANEL_STATE_HIDE;
    iseContext.language     = ECORE_IMF_INPUT_PANEL_LANG_AUTOMATIC;
    iseContext.orient       = 0;
    iseContext.fUseImEffect = TRUE;

    if (!IfInitContext) {
        IfInitContext = true;
    }

    IMFCONTROLUIDBG("debug end\n");
}

static Eina_Bool _hide_timer_handler (void *data)
{
    LOGD("input panel hide. ctx : %p\n", data);
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

    if (iseContext.state == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
        hide_req_ic = ctx;
    }

    iseContext.IfAlwaysShow = FALSE;

    if (instant) {
        if (hide_timer) {
            ecore_timer_del (hide_timer);
            hide_timer = NULL;
        }

        LOGD("input panel hide. ctx : %p\n", ctx);
        _isf_imf_context_input_panel_hide ();
    } else {
        _input_panel_hide_timer_start (ctx);
    }
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
}

EAPI void isf_imf_input_panel_shutdown (void)
{
    IMFCONTROLUIDBG("[%s]\n", __FUNCTION__);

    if (_prop_change_handler) {
        ecore_event_handler_del (_prop_change_handler);
        _prop_change_handler = NULL;
    }

    if (hide_timer) {
        ecore_timer_del (hide_timer);
        hide_timer = NULL;
    }
}

EAPI void isf_imf_context_input_panel_show (Ecore_IMF_Context* ctx)
{
    int   length = -1;
    int   i = 0;
    Disable_Key_Item *dkey_item = NULL;
    Private_Key_Item *pkey_item = NULL;
    Eina_List *disable_key_list = NULL;
    Eina_List *private_key_list = NULL;
    Eina_List *l = NULL;
    void *list_data = NULL;
    void *offset = NULL;
    void *packet = NULL;

    input_panel_ctx = ctx;

    IMFCONTROLUIDBG("debug start --%s\n", __FUNCTION__);

    if (hw_kbd_num != 0) {
        printf ("H/W keyboard is existed.\n");
        return;
    }

    if (hide_timer) {
        ecore_timer_del(hide_timer);
        hide_timer = NULL;
    }

    if (IfInitContext == false) {
        _isf_imf_context_init ();
    }

    show_req_ic = ctx;

    /* get input language */
    iseContext.language = ecore_imf_context_input_panel_language_get (ctx);
    IMFCONTROLUIDBG("[%s] language : %d\n", __func__, iseContext.language);

    /* get layout */
    iseContext.layout = ecore_imf_context_input_panel_layout_get (ctx);
    IMFCONTROLUIDBG("[%s] layout : %d\n", __func__, iseContext.layout);

    /* get language */
    iseContext.language = ecore_imf_context_input_panel_language_get (ctx);
    IMFCONTROLUIDBG("[%s] language : %d\n", __func__, iseContext.language);

    /* get disable key list */
    disable_key_list = ecore_imf_context_input_panel_key_disabled_list_get (ctx);
    iseContext.disabled_key_num = eina_list_count (disable_key_list);
    IMFCONTROLUIDBG("disable key_num : %d\n", iseContext.disabled_key_num);

    /* get private key list */
    private_key_list = ecore_imf_context_input_panel_private_key_list_get (ctx);
    iseContext.private_key_num = eina_list_count (private_key_list);
    IMFCONTROLUIDBG("private key_num : %d\n", iseContext.private_key_num);

    /* calculate packet size */
    length = sizeof (iseContext);
    length += iseContext.disabled_key_num * sizeof (Disable_Key_Item);
    length += iseContext.private_key_num * sizeof (Private_Key_Item);

    /* create packet */
    packet = calloc (1, length);
    if (!packet)
        return;

    memcpy (packet, (void *)&iseContext, sizeof (iseContext));

    i = 0;
    offset = (void *)((unsigned int)packet + sizeof (iseContext));
    EINA_LIST_FOREACH(disable_key_list, l, list_data) {
        dkey_item = (Disable_Key_Item *)list_data;

        memcpy ((void *)((unsigned int)offset+i*sizeof(Disable_Key_Item)), dkey_item, sizeof (Disable_Key_Item));
        IMFCONTROLUIDBG("[Disable Key] layout : %d, key : %d, disable : %d\n", dkey_item->layout_idx, dkey_item->key_idx, dkey_item->disabled);
        i++;
    }

    offset = (void *)((unsigned int)offset + iseContext.disabled_key_num * sizeof (Disable_Key_Item));

    i = 0;
    EINA_LIST_FOREACH(private_key_list, l, list_data) {
        pkey_item = (Private_Key_Item *)list_data;
        memcpy ((void *)((unsigned int)offset + i * sizeof (Private_Key_Item)), pkey_item, sizeof (Private_Key_Item));
        IMFCONTROLUIDBG("[Private Key] layout : %d, key : %d, label : %s, key_value : %d, key_str : %s\n", pkey_item->layout_idx, pkey_item->key_idx, pkey_item->data, pkey_item->key_value, pkey_item->key_string);
        i++;
    }

    /* Set the current XID of the active window into the root window property */
    _save_current_xid (ctx);

    iseContext.state = ECORE_IMF_INPUT_PANEL_STATE_SHOW;

    LOGD("input panel show. ctx : %p\n", ctx);

    _isf_imf_context_input_panel_show (packet ,length);
    free (packet);

    send_caps_mode (ctx);
}

EAPI void isf_imf_context_input_panel_hide (Ecore_IMF_Context *ctx)
{
    IMFCONTROLUIDBG("[%s]\n", __func__);

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

EAPI void isf_imf_context_input_panel_caps_mode_set (Ecore_IMF_Context *ctx, unsigned int mode)
{
    IMFCONTROLUIDBG("[%s] shift mode : %d\n", __func__, mode);

    if (!IfInitContext)
        _isf_imf_context_init ();
    _isf_imf_context_input_panel_caps_mode_set (mode);
}

/**
 * Set up an ISE specific data
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[in] data pointer of data to sets up to ISE
 * @param[in] length length of data
 */
EAPI void isf_imf_context_input_panel_imdata_set (Ecore_IMF_Context *ctx, const char* data, int length)
{
    IMFCONTROLUIDBG("[%s] data : %s, len : %d\n", __func__, data, length);

    if (length < 0)
        return;
    if (!IfInitContext)
        _isf_imf_context_init ();
    _isf_imf_context_input_panel_imdata_set (data, length);
}

/**
 * Get the ISE specific data from ISE
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[out] data pointer of data to return
 * @param[out] length length of data
 */
EAPI void isf_imf_context_input_panel_imdata_get (Ecore_IMF_Context *ctx, char* data, int* length)
{
    if (!IfInitContext)
        _isf_imf_context_init ();
    _isf_imf_context_input_panel_imdata_get (data, length);
    IMFCONTROLUIDBG("[%s] imdata : %s, len : %d\n", __func__, data, *length);
}

EAPI void isf_imf_context_input_panel_move (Ecore_IMF_Context *ctx, int x, int y)
{
    IMFCONTROLUIDBG("[%s] x : %d, y : %d\n", __func__, x, y);

    if (!IfInitContext)
        _isf_imf_context_init ();
    iseContext.input_panel_x = x;
    iseContext.input_panel_x = y;
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

    IMFCONTROLUIDBG("[%s] x : %d, y : %d, w : %d, h : %d\n", __func__, *x, *y, *w, *h);
}

/**
 * Sets up a private key in active ISE keyboard layout for own's application.
 * In some case which does not support the private key will be ignored even 
 * through the application requested.
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[in] layout_idx an index of layout page to be set
 * @param[in] key_idx an index of key to be set
 * @param[in] img_path the image file to be set
 * @param[in] label a text label to be displayed on private key
 * @param[in] value a value of key. If null, it will use original value of key
 */
EAPI void isf_imf_context_input_panel_private_key_set (Ecore_IMF_Context *ctx,
                                                       int                layout_index,
                                                       int                key_index,
                                                       const char        *img_path,
                                                       const char        *label,
                                                       const char        *value)
{
    IMFCONTROLUIDBG("[%s] layout : %d, key_index : %d, img_path : %s, label : %s, value : %s\n", __func__, layout_index, key_index, img_path, label, value);

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();

    if (context_scim != get_focused_ic ())
        return;

    if (img_path) {
        _isf_imf_context_input_panel_private_key_set_by_image (layout_index, key_index, img_path, value);
    } else {
        _isf_imf_context_input_panel_private_key_set (layout_index, key_index, label, value);
    }
}

/**
 * Make a key to be disabled in active ISE keyboard layout for own's application.
 * In some case which does not support the disable key will be ignored
 * even through the application requested.
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[in] layout_idx an index of layout page to be set
 * @param[in] key_idx an index of key to be set
 * @param[in] disabled the state
 */
EAPI void isf_imf_context_input_panel_key_disabled_set (Ecore_IMF_Context *ctx, int layout_index, int key_index, Eina_Bool disabled)
{
    IMFCONTROLUIDBG("[%s] layout : %d, key_index : %d, value : %d\n", __func__, layout_index, key_index, disabled);

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();

    if (context_scim == get_focused_ic())
        _isf_imf_context_input_panel_key_disabled_set (layout_index, key_index, disabled);
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
    IMFCONTROLUIDBG("[%s] state : %d\n", __func__, iseContext.state);

    if (!IfInitContext)
        _isf_imf_context_init ();
    return iseContext.state;
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

    IMFCONTROLUIDBG("[%s]\n", __func__);

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

    for (l = EventCallbackList; l;) {
        fn = (EventCallbackNode *)l->data;

        if ((fn) && (fn->func == func) && (fn->type == type)) {
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

    IMFCONTROLUIDBG("[%s]\n", __func__);

    for (l = EventCallbackList; l;) {
        fn = (EventCallbackNode *)l->data;

        if ((fn) && (fn->imf_context == ctx)) {
            EventCallbackList = eina_list_remove (EventCallbackList, fn);
            free (fn);
        }
        l = l->next;
    }
}

/**
 * process command message, ISM_TRANS_CMD_ISE_PANEL_SHOWED of ecore_ise_process_event()
 */
static bool _process_ise_panel_showed (void)
{
    /* When the size of ISE gets changed, STATE_SHOW is be delivered again to letting applications know the change.
       Later, an event type for notifying the size change of ISE needs to be added instead. */
    iseContext.state = ECORE_IMF_INPUT_PANEL_STATE_SHOW;

    /* Notify that ISE status has changed */
    _event_callback_call (ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_SHOW);

    return true;
}

/**
 * process command message, ISM_TRANS_CMD_ISE_PANEL_HIDED of ecore_ise_process_event()
 */
static bool _process_ise_panel_hided (void)
{
    if (iseContext.state == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
        IMFCONTROLUIDBG("ISE is already hided (_process_ise_panel_hided)\n");
        return false;
    }

    iseContext.state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;

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

    if (type == (uint32)ECORE_IMF_INPUT_PANEL_STATE_EVENT && value == (uint32)ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
        _process_ise_panel_hided ();
        return true;
    }

    if (type == (uint32)ECORE_IMF_INPUT_PANEL_STATE_EVENT && value == (uint32)ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
        _process_ise_panel_showed ();
        return true;
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
    default :
        IMFCONTROLUIDBG("unknown command");
        break;
    }
}

