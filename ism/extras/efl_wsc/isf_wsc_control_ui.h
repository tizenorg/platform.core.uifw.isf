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

#ifndef __ISF_IMF_CONTROL_UI_H
#define __ISF_IMF_CONTROL_UI_H

#define Uses_SCIM_PANEL_CLIENT

#include "isf_wsc_context.h"
#include <Ecore_IMF.h>
#include "scim.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    int  hw_keyboard_num_get ();
    bool process_update_input_context (int type, int value);
    void clear_hide_request ();

    void isf_wsc_input_panel_init ();
    void isf_wsc_input_panel_shutdown ();
    void isf_wsc_context_input_panel_show (WSCContextISF *ctx);
    void isf_wsc_context_input_panel_hide (WSCContextISF *ctx);
    void isf_wsc_context_input_panel_language_set (WSCContextISF *ctx, Ecore_IMF_Input_Panel_Lang lang);
    Ecore_IMF_Input_Panel_Lang isf_wsc_context_input_panel_language_get (WSCContextISF *ctx);
    void isf_wsc_context_input_panel_layout_set (WSCContextISF *ctx, Ecore_IMF_Input_Panel_Layout layout);
    Ecore_IMF_Input_Panel_Layout isf_wsc_context_input_panel_layout_get (WSCContextISF *ctx);
    void isf_wsc_context_input_panel_caps_mode_set (WSCContextISF *ctx, unsigned int mode);
    void isf_wsc_context_input_panel_caps_lock_mode_set (WSCContextISF *ctx, Eina_Bool mode);
    void isf_wsc_context_set_keyboard_mode (WSCContextISF *ctx, scim::TOOLBAR_MODE_T mode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ISF_IMF_CONTROL_UI_H */

