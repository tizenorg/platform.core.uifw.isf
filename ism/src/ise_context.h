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

#ifndef __ISE_CONTEXT_H
#define __ISE_CONTEXT_H

#include <Ecore_IMF.h>

typedef struct {
    char name[_POSIX_PATH_MAX];
    int IfAlwaysShow;
    int IfFullStyle;
    Ecore_IMF_Input_Panel_State state;
    int fUseImEffect;
    Ecore_IMF_Input_Panel_Lang language;
    Ecore_IMF_Input_Panel_Layout layout;
    Ecore_IMF_Input_Panel_Orient orient;
    unsigned int disabled_key_num;
    unsigned int private_key_num;
    int input_panel_x;
    int input_panel_y;
} Ise_Context;

#endif  /* __ISE_CONTEXT_H */

/*
vi:ts=4:expandtab:nowrap
*/
