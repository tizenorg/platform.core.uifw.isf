/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *
 * Contact: Jihoon Kim <jihoon48.kim@samsung.com>
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

#include "isf_demo_efl.h"
#include "isf_password_mode_efl.h"

static void
ck_changed_cb (void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *en;
    en = (Evas_Object*)data;

    elm_entry_password_set (en, elm_check_state_get (obj));
}

static Evas_Object * create_inner_layout (void *data)
{
    struct appdata *ad = (struct appdata *)data;
    Evas_Object *bx = NULL;
    Evas_Object *ef = NULL;
    Evas_Object *en = NULL;

    Evas_Object *parent = ad->naviframe;

    bx = elm_box_add (parent);
    evas_object_size_hint_weight_set (bx, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (bx, EVAS_HINT_FILL, 0.0);
    evas_object_show (bx);

    ef = create_ef (parent, _("Password mode"), _("click to enter"), &en);
    elm_entry_password_set (en, EINA_TRUE);

    elm_box_pack_end (bx, ef);

    Evas_Object *ck = elm_check_add (parent);
    evas_object_size_hint_weight_set (ck, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set (ck, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_check_state_set (ck, EINA_TRUE);
    evas_object_smart_callback_add (ck, "changed", ck_changed_cb, en);
    elm_object_text_set (ck, _("Password mode"));
    elm_object_focus_allow_set (ck, EINA_FALSE);
    elm_box_pack_end (bx, ck);
    evas_object_show (ck);

    return bx;
}

void ise_password_mode_bt (void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *lay_inner = create_inner_layout (data);
    add_layout_to_naviframe (data, lay_inner, _("Password mode"));
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
