/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Shuo Liu <shuo0805.liu@samsung.com>, Hengliang Luo <hl.luo@samsung.com>
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

#ifndef __ISF_SETTING_WIZARD_H
#define __ISF_SETTING_WIZARD_H

#define ISF_HEADER_CANCEL_ICON                    (SCIM_ICONDIR "/01_header_icon_cancel.png")
#define ISF_HEADER_DONE_ICON                      (SCIM_ICONDIR "/01_header_icon_done.png")

struct ug_data {
    Evas_Object *layout_main;
    Evas_Object *naviframe;
    bundle *data; 
    struct ui_gadget *ug;
};

#endif /* __ISF_SETTING_WIZARD_H */

/*
vi:ts=4:ai:nowrap:expandtab
*/

