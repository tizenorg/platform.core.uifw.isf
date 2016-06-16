/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
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
#define Uses_SCIM_CONFIG_PATH

/* IM control UI part */
#include "scim_private.h"

#include <Ecore_Wayland.h>
#include <Ecore_IMF.h>
#ifdef HAVE_VCONF
#include <vconf.h>
#endif

#include "scim.h"
#include "isf_wsc_control_ui.h"
#include "isf_wsc_context.h"
#include "isf_wsc_control.h"
#include "ise_context.h"
#include "isf_debug.h"

#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG                                         "ISF_WSC_EFL"

using namespace scim;

/* IM control related variables */
static TOOLBAR_MODE_T     kbd_mode = TOOLBAR_HELPER_MODE;
WSCContextISF            *input_panel_ctx = NULL;
static bool               _support_hw_keyboard_mode = false;

static int _get_context_id (WSCContextISF *ctx)
{
    WSCContextISF *context_scim = ctx;

    if (!context_scim) return -1;

    return context_scim->id;
}

#ifdef HAVE_VCONF
static void keyboard_mode_changed_cb (keynode_t *key, void* data)
{
    int val;
    if (vconf_get_bool (VCONFKEY_ISF_HW_KEYBOARD_INPUT_DETECTED, &val) != 0) {
        LOGW ("Failed to get input detect\n");
        return;
    }

    kbd_mode = (TOOLBAR_MODE_T)!val;
}
#endif

TOOLBAR_MODE_T get_keyboard_mode ()
{
    return kbd_mode;
}

void isf_wsc_input_panel_init (void)
{
    _support_hw_keyboard_mode = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_SUPPORT_HW_KEYBOARD_MODE), _support_hw_keyboard_mode);

#ifdef HAVE_VCONF
    vconf_notify_key_changed (VCONFKEY_ISF_HW_KEYBOARD_INPUT_DETECTED, keyboard_mode_changed_cb, NULL);
#endif
}

void isf_wsc_input_panel_shutdown (void)
{
#ifdef HAVE_VCONF
    vconf_ignore_key_changed (VCONFKEY_ISF_HW_KEYBOARD_INPUT_DETECTED, keyboard_mode_changed_cb);
#endif
}

void isf_wsc_context_input_panel_show (WSCContextISF* wsc_ctx)
{
    int length = -1;
    void *packet = NULL;
    char imdata[1024] = {0};
    bool input_panel_show = false;
    input_panel_ctx = wsc_ctx;
    Ise_Context iseContext;

    if (!wsc_ctx || !wsc_ctx->ctx)
        return;

    /* set password mode */
    iseContext.password_mode = wsc_context_input_panel_password_mode_get (wsc_ctx);

    /* set language */
    iseContext.language = wsc_context_input_panel_language_get (wsc_ctx);

    /* set layout in ise context info */
    iseContext.layout = wsc_context_input_panel_layout_get (wsc_ctx);

    /* set layout variation in ise context info */
    iseContext.layout_variation = wsc_context_input_panel_layout_variation_get (wsc_ctx);

    /* set prediction allow */
    iseContext.prediction_allow = wsc_context_prediction_allow_get (wsc_ctx);

    if (iseContext.layout == ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD)
        iseContext.password_mode = EINA_TRUE;

    if (iseContext.password_mode)
        iseContext.prediction_allow = EINA_FALSE;

    if (iseContext.layout == ECORE_IMF_INPUT_PANEL_LAYOUT_TERMINAL)
        iseContext.prediction_allow = EINA_FALSE;

    /* set return key type */
    iseContext.return_key_type = (Ecore_IMF_Input_Panel_Return_Key_Type)wsc_ctx->return_key_type;

    /* set return key disabled */
    iseContext.return_key_disabled = wsc_ctx->return_key_disabled;

    /* set caps mode */
    iseContext.caps_mode = caps_mode_check (wsc_ctx, EINA_TRUE, EINA_FALSE);

    /* set client window */
    iseContext.client_window = 0;

    /* set the size of imdata */
    context_scim_imdata_get (wsc_ctx, (void *)imdata, &iseContext.imdata_size);

    /* set the cursor position of the editable widget */
    wsc_context_surrounding_get (wsc_ctx, NULL, &iseContext.cursor_pos);

    iseContext.autocapital_type = wsc_context_autocapital_type_get (wsc_ctx);

    iseContext.input_hint = wsc_context_input_hint_get (wsc_ctx);

    /* FIXME */
    iseContext.bidi_direction = ECORE_IMF_BIDI_DIRECTION_NEUTRAL;

    LOGD ("ctx : %p, layout : %d, layout variation : %d\n", wsc_ctx, iseContext.layout, iseContext.layout_variation);
    LOGD ("language : %d, cursor position : %d, caps mode : %d\n", iseContext.language, iseContext.cursor_pos, iseContext.caps_mode);
    LOGD ("return_key_type : %d, return_key_disabled : %d, autocapital type : %d\n", iseContext.return_key_type, iseContext.return_key_disabled, iseContext.autocapital_type);
    LOGD ("password mode : %d, prediction_allow : %d\n", iseContext.password_mode, iseContext.prediction_allow);
    LOGD ("input hint : %#x\n", iseContext.input_hint);
    LOGD ("bidi direction : %d\n", iseContext.bidi_direction);

    /* calculate packet size */
    length = sizeof (iseContext);
    length += iseContext.imdata_size;

    /* create packet */
    packet = calloc (1, length);
    if (!packet)
        return;

    memcpy (packet, (void *)&iseContext, sizeof (iseContext));
    memcpy ((void *)((char *)packet + sizeof (iseContext)), (void *)imdata, iseContext.imdata_size);

    int context_id = _get_context_id (wsc_ctx);

    _isf_wsc_context_input_panel_show (get_panel_client_id (), context_id, packet, length, input_panel_show);

    free (packet);

    caps_mode_check (wsc_ctx, EINA_TRUE, EINA_TRUE);

#if HAVE_VCONF
    vconf_set_int (VCONFKEY_ISF_INPUT_PANEL_STATE, VCONFKEY_ISF_INPUT_PANEL_STATE_SHOW);
#endif
}

void isf_wsc_context_input_panel_hide (WSCContextISF *ctx)
{
    int context_id = _get_context_id (ctx);
    _isf_wsc_context_input_panel_hide (get_panel_client_id (), context_id);

#if HAVE_VCONF
    vconf_set_int (VCONFKEY_ISF_INPUT_PANEL_STATE, VCONFKEY_ISF_INPUT_PANEL_STATE_HIDE);
#endif
}

void isf_wsc_context_set_keyboard_mode (WSCContextISF *ctx, TOOLBAR_MODE_T mode)
{
    _support_hw_keyboard_mode = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_SUPPORT_HW_KEYBOARD_MODE), _support_hw_keyboard_mode);

    if (_support_hw_keyboard_mode) {
        kbd_mode = mode;
        SECURE_LOGD ("keyboard mode : %d\n", kbd_mode);
        _isf_wsc_context_set_keyboard_mode (_get_context_id (ctx), mode);
    }
}

void
isf_wsc_context_input_panel_layout_set (WSCContextISF *ctx, Ecore_IMF_Input_Panel_Layout layout)
{
    WSCContextISF *context_scim = ctx;

    if (context_scim == get_focused_ic ()) {
        LOGD ("layout type : %d\n", layout);
        _isf_wsc_context_input_panel_layout_set (_get_context_id (ctx), layout);
    }
}

Ecore_IMF_Input_Panel_Layout isf_wsc_context_input_panel_layout_get (WSCContextISF *ctx)
{
    Ecore_IMF_Input_Panel_Layout layout;
    _isf_wsc_context_input_panel_layout_get (_get_context_id (ctx), &layout);

    return layout;
}

void isf_wsc_context_input_panel_caps_mode_set (WSCContextISF *ctx, unsigned int mode)
{
    LOGD ("ctx : %p, mode : %d\n", ctx, mode);
    _isf_wsc_context_input_panel_caps_mode_set (_get_context_id (ctx), mode);
}

void isf_wsc_context_input_panel_caps_lock_mode_set (WSCContextISF *ctx, Eina_Bool mode)
{
    WSCContextISF *context_scim = ctx;

    if (context_scim == get_focused_ic ())
        caps_mode_check (ctx, EINA_TRUE, EINA_TRUE);
}

void isf_wsc_context_input_panel_language_set (WSCContextISF *ctx, Ecore_IMF_Input_Panel_Lang language)
{
    WSCContextISF *context_scim = ctx;

    if (context_scim == get_focused_ic ()) {
        LOGD ("language mode : %d\n", language);
        _isf_wsc_context_input_panel_language_set (_get_context_id (ctx), language);
    }
}

void isf_wsc_context_input_panel_return_key_type_set (WSCContextISF *ctx, Ecore_IMF_Input_Panel_Return_Key_Type return_key_type)
{
    LOGD ("ctx : %p, return key type : %d\n", ctx, return_key_type);
    _isf_wsc_context_input_panel_return_key_type_set (_get_context_id (ctx), return_key_type);
}

void isf_wsc_context_input_panel_return_key_disabled_set (WSCContextISF *ctx, Eina_Bool disabled)
{
    LOGD ("ctx : %p, disabled : %d\n", ctx, disabled);
    _isf_wsc_context_input_panel_return_key_disabled_set (_get_context_id (ctx), disabled);
}

void isf_wsc_context_input_panel_imdata_set (WSCContextISF *ctx, const void *imdata, int len)
{
    _isf_wsc_context_input_panel_imdata_set (_get_context_id (ctx), imdata, len);
}

void isf_wsc_context_input_panel_imdata_get (WSCContextISF *ctx, void **imdata, int* len)
{
    _isf_wsc_context_input_panel_imdata_get (_get_context_id (ctx), imdata, len);
}

void isf_wsc_context_process_input_device_event (WSCContextISF *ctx, uint32_t type, const char *data, uint32_t len)
{
    _isf_wsc_context_process_input_device_event (_get_context_id(ctx), type, data, len);
}
