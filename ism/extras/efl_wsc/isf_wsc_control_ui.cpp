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
#include <Ecore.h>
#include <Ecore_Wayland.h>
#include <Ecore_Evas.h>
#include <Ecore_IMF.h>
#include <glib.h>
#include <text-client-protocol.h>

#include "scim.h"
#include "isf_wsc_control_ui.h"
#include "isf_wsc_context.h"
#include "isf_wsc_control.h"
#include "ise_context.h"

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

TOOLBAR_MODE_T get_keyboard_mode ()
{
    return kbd_mode;
}

void isf_wsc_input_panel_init (void)
{
    _support_hw_keyboard_mode = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_SUPPORT_HW_KEYBOARD_MODE), _support_hw_keyboard_mode);
}

void isf_wsc_input_panel_shutdown (void)
{
}

void isf_wsc_context_input_panel_show (WSCContextISF* ctx)
{
    int length = -1;
    void *packet = NULL;
    char imdata[1024] = {0};
    bool input_panel_show = false;
    input_panel_ctx = ctx;
    Ise_Context iseContext;

    if (!ctx || !ctx->ctx)
        return;

    if (kbd_mode == TOOLBAR_KEYBOARD_MODE) {
        LOGD ("H/W keyboard is existed.\n");
        return;
    }

    /* set password mode */
    iseContext.password_mode = wsc_context_input_panel_password_mode_get (ctx->ctx);

    /* set language */
    iseContext.language = wsc_context_input_panel_language_get (ctx->ctx);

    /* set layout in ise context info */
    iseContext.layout = wsc_context_input_panel_layout_get (ctx->ctx);

    /* set layout variation in ise context info */
    iseContext.layout_variation = wsc_context_input_panel_layout_variation_get (ctx->ctx);

    /* set prediction allow */
    iseContext.prediction_allow = wsc_context_prediction_allow_get (ctx->ctx);

    if (iseContext.layout == ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD)
        iseContext.password_mode = EINA_TRUE;

    if (iseContext.password_mode)
        iseContext.prediction_allow = EINA_FALSE;

    if (iseContext.layout == ECORE_IMF_INPUT_PANEL_LAYOUT_TERMINAL)
        iseContext.prediction_allow = EINA_FALSE;

    isf_wsc_context_prediction_allow_set (ctx, iseContext.prediction_allow);

    /* set return key type */
    iseContext.return_key_type = (Ecore_IMF_Input_Panel_Return_Key_Type)ctx->ctx->return_key_type;

    /* set return key disabled */
    iseContext.return_key_disabled = ctx->ctx->return_key_disabled;

    /* set caps mode */
    iseContext.caps_mode = caps_mode_check (ctx, EINA_TRUE, EINA_FALSE);

    /* set client window */
    iseContext.client_window = 0;

    /* set the size of imdata */
    context_scim_imdata_get (ctx, (void *)imdata, &iseContext.imdata_size);

    /* set the cursor position of the editable widget */
    //ecore_imf_context_surrounding_get (ctx, NULL, &iseContext.cursor_pos);
    iseContext.cursor_pos = 0;

    iseContext.autocapital_type = wsc_context_autocapital_type_get (ctx->ctx);

    iseContext.input_hint = wsc_context_input_hint_get (ctx->ctx);

    LOGD ("ctx : %p, layout : %d, layout variation : %d\n", ctx, iseContext.layout, iseContext.layout_variation);
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

    int context_id = _get_context_id (ctx);

    _isf_wsc_context_input_panel_show (get_panel_client_id (), context_id, packet, length, input_panel_show);

    free (packet);

    caps_mode_check (ctx, EINA_TRUE, EINA_TRUE);
}

void isf_wsc_context_input_panel_hide (WSCContextISF *ctx)
{
    int context_id = _get_context_id (ctx);
    _isf_wsc_context_input_panel_hide (get_panel_client_id (), context_id);
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
        imengine_layout_set (ctx, layout);
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
