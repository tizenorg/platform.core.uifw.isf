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

#ifndef __ISF_SETTING_EFL_H
#define __ISF_SETTING_EFL_H
enum
{
    TYPE_KEY_END = 0,
    //add more here
};

struct ug_data {
    Evas_Object *layout_main;
    Evas_Object *naviframe;
    Evas_Object *opt_eo;
    Elm_Object_Item *sw_ise_item_tizen; //sw
    Elm_Object_Item *hw_ise_item_tizen; //hw
    Elm_Object_Item *sw_ise_opt_item_tizen; //sw opt
    Elm_Object_Item *hw_ise_opt_item_tizen; //hw opt
    void (*key_end_cb)(void *, Evas_Object *, void *);
    struct ui_gadget *ug;
};

#endif /* __ISF_SETTING_EFL_H */

/*
vi:ts=4:ai:nowrap:expandtab
*/
