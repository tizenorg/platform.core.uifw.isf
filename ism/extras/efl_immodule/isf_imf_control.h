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

#ifndef __ISF_IMF_CONTROL_H
#define __ISF_IMF_CONTROL_H

#include <Ecore_IMF.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /* non UI related works */
    int _isf_imf_context_input_panel_show (void *data, int length);
    int _isf_imf_context_input_panel_hide (void);
    int _isf_imf_context_control_panel_show (void);
    int _isf_imf_context_control_panel_hide (void);

    int _isf_imf_context_input_panel_language_set (Ecore_IMF_Input_Panel_Lang lang);

    int _isf_imf_context_input_panel_imdata_set (const char *data, int len);
    int _isf_imf_context_input_panel_imdata_get (char *data, int *len);
    int _isf_imf_context_input_panel_geometry_get (int *x, int *y, int *w, int *h);
    int _isf_imf_context_input_panel_private_key_set (int layout_index, int key_index, const char *label, const char *value);
    int _isf_imf_context_input_panel_private_key_set_by_image (int layout_index, int key_index, const char *img_path, const char *value);
    int _isf_imf_context_input_panel_layout_set (Ecore_IMF_Input_Panel_Layout layout);
    int _isf_imf_context_input_panel_layout_get (Ecore_IMF_Input_Panel_Layout *layout);
    int _isf_imf_context_input_panel_key_disabled_set (int layout_index, int key_index, Eina_Bool disabled);
    int _isf_imf_context_input_panel_caps_mode_set (unsigned int mode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ISF_IMF_CONTROL_H */

