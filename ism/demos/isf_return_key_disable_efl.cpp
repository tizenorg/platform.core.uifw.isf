/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Shuo Liu <shuo0805.liu@samsung.com>, Jihoon Kim <jihoon48.kim@samsung.com>
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
#include "isf_return_key_disable_efl.h"

static void
_cursor_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *en = obj;

    int pos = elm_entry_cursor_pos_get (en);

    if (pos == 0) {
        elm_entry_input_panel_return_key_disabled_set (en, EINA_TRUE);
    }
    else {
        elm_entry_input_panel_return_key_disabled_set (en, EINA_FALSE);
    }
}

static Evas_Object *_create_ef_layout (Evas_Object *parent, const char *label, const char *guide_text)
{
    Evas_Object *ef = create_ef (parent, label, guide_text);
    Evas_Object *en = elm_object_part_content_get (ef, "elm.swallow.content");
    evas_object_smart_callback_add (en, "cursor,changed", _cursor_changed_cb, NULL);

    return ef;
}

static Evas_Object * create_inner_layout (void *data)
{
    struct appdata *ad = (struct appdata *)data;
    Evas_Object *bx = NULL;
    Evas_Object *ef = NULL;

    Evas_Object *parent = ad->naviframe;

    bx = elm_box_add (parent);
    evas_object_size_hint_weight_set (bx, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (bx, EVAS_HINT_FILL, 0.0);
    evas_object_show (bx);

    /* Prediction allow : TRUE */
    ef = _create_ef_layout (parent, _("Return Key Disable"), _("Return Key will be activated when any key is pressed"));
    elm_box_pack_end (bx, ef);

    return bx;
}

void ise_return_key_disable_bt (void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *lay_inner = create_inner_layout (data);
    add_layout_to_naviframe (data, lay_inner, _("Return Key Disable"));
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
