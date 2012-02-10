/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
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

static Ecore_IMF_Context *imf_context = NULL;
static Elm_Genlist_Item_Class itci;

enum {
    INPUT_PANEL_GEOMETRY_GET,
    INPUT_PANEL_SHOW,
    INPUT_PANEL_HIDE,
    INPUT_PANEL_IMDATA_SET,
    INPUT_PANEL_IMDATA_GET,
    INPUT_PANEL_LAYOUT_SET,
    INPUT_PANEL_LAYOUT_GET,
    INPUT_PANEL_PRIVATE_KEY_SET,
    INPUT_PANEL_KEY_DISABLED_SET,
    INPUT_PANEL_STATE_GET,
    CONTROL_PANEL_SHOW,
    CONTROL_PANEL_HIDE,
};

const char *api_list[]={
    "PANEL GEOMETRY GET",
    "INPUT PANEL SHOW",
    "INPUT PANEL HIDE",
    "INPUT PANEL IMDATA SET",
    "INPUT PANEL IMDATA GET",
    "INPUT PANEL LAYOUT SET",
    "INPUT PANEL LAYOUT GET",
    "PANEL PRIVATE KEY SET",
    "PANEL KEY DISABLED SET",
    "INPUT PANEL STATE GET",
    "CTRL PANEL SHOW",
    "CTRL PANEL HIDE",
};

static void test_input_panel_geometry_get (void *data, Evas_Object *obj, void *event_info)
{
    int x, y, w, h;
    if (imf_context != NULL) {
        ecore_imf_context_input_panel_geometry_get (imf_context, &x, &y, &w, &h);
        printf ("x=%d \n", x);
        printf ("y=%d \n", y);
        printf ("width=%d \n", w);
        printf ("height=%d \n", h);
    }
}

void test_input_panel_show (void *data, Evas_Object *obj, void *event_info)
{
    if (imf_context != NULL) {
        ecore_imf_context_input_panel_show (imf_context);
    }
}

void test_input_panel_hide (void *data, Evas_Object *obj, void *event_info)
{
    if (imf_context != NULL) {
        ecore_imf_context_input_panel_hide (imf_context);
    }
}

void test_input_panel_imdata_set (void *data, Evas_Object *obj, void *event_info)
{
    // need ISE to deal with the data
    char buf[256] = "ur imdata";
    if (imf_context != NULL)
        ecore_imf_context_input_panel_imdata_set (imf_context, buf, sizeof (buf));
}

void test_input_panel_imdata_get (void *data, Evas_Object *obj, void *event_info)
{
    int len = 256;
    char* buf = (char*) malloc (len);
    if (buf != NULL) {
        memset (buf, '\0', len);
        if (imf_context != NULL) {
            ecore_imf_context_input_panel_imdata_get (imf_context, buf, &len);
            printf ("get imdata  %s, and len is %d ...\n", (char *)buf, len);
        }
        free (buf);
    }
}

void test_input_panel_layout_set (void *data, Evas_Object *obj, void *event_info)
{
    Ecore_IMF_Input_Panel_Layout layout = ECORE_IMF_INPUT_PANEL_LAYOUT_EMAIL;
    if (imf_context != NULL)
        ecore_imf_context_input_panel_layout_set (imf_context, layout);
}

void test_input_panel_layout_get (void *data, Evas_Object *obj, void *event_info)
{
    Ecore_IMF_Input_Panel_Layout layout;
    if (imf_context != NULL) {
        layout = ecore_imf_context_input_panel_layout_get (imf_context);
        printf ("get layout : %d ...\n", (int)layout);
    }
}

void test_input_panel_private_key_set (void *data, Evas_Object *obj, void *event_info)
{
    int layout_index = 1;

    if (imf_context != NULL)
        ecore_imf_context_input_panel_private_key_set (imf_context, layout_index, ECORE_IMF_INPUT_PANEL_KEY_ENTER, NULL, "Go", ECORE_IMF_INPUT_PANEL_KEY_ENTER, NULL);
}

void test_input_panel_key_disabled_set (void *data, Evas_Object *obj, void *event_info)
{
    int layout_index = 1;
    int key_index    = 1;

    if (imf_context != NULL)
        ecore_imf_context_input_panel_key_disabled_set (imf_context, layout_index, key_index, EINA_TRUE);
}

void test_input_panel_state_get (void *data, Evas_Object *obj, void *event_info)
{
    Ecore_IMF_Input_Panel_State state;

    if (imf_context != NULL) {
        state = ecore_imf_context_input_panel_state_get (imf_context);
        printf ("ise state : %d \n", (int)state);
    }
}

void test_control_panel_show (void *data, Evas_Object *obj, void *event_info)
{
    if (imf_context != NULL) {
        ecore_imf_context_control_panel_show (imf_context);
    }
}

void test_control_panel_hide (void *data, Evas_Object *obj, void *event_info)
{
    if (imf_context != NULL) {
        ecore_imf_context_control_panel_hide (imf_context);
    }
}

char *gli_label_get (void *data, Evas_Object *obj, const char *part)
{
    int j = (int)data;
    return strdup (api_list[j]);
}

static void test_api (void *data, Evas_Object *obj, void *event_info)
{
    int j = (int)data;
    switch (j) {
    case INPUT_PANEL_GEOMETRY_GET:
        test_input_panel_geometry_get (NULL, obj, event_info);
        break;
    case INPUT_PANEL_SHOW:
        test_input_panel_show (NULL, obj, event_info);
        break;
    case INPUT_PANEL_HIDE:
        test_input_panel_hide (NULL, obj, event_info);
        break;
    case INPUT_PANEL_IMDATA_SET:
        test_input_panel_imdata_set (NULL,obj, event_info);
        break;
    case INPUT_PANEL_IMDATA_GET:
        test_input_panel_imdata_get (NULL,obj, event_info);
        break;
    case INPUT_PANEL_LAYOUT_SET:
        test_input_panel_layout_set (NULL,obj, event_info);
        break;
    case INPUT_PANEL_LAYOUT_GET:
        test_input_panel_layout_get (NULL,obj, event_info);
        break;
    case INPUT_PANEL_PRIVATE_KEY_SET:
        test_input_panel_private_key_set (NULL, obj, event_info);
        break;
    case INPUT_PANEL_KEY_DISABLED_SET:
        test_input_panel_key_disabled_set (NULL, obj, event_info);
        break;
    case INPUT_PANEL_STATE_GET:
        test_input_panel_state_get (NULL, obj, event_info);
        break;
    case CONTROL_PANEL_SHOW:
        test_control_panel_show (NULL, obj, event_info);
        break;
    case CONTROL_PANEL_HIDE:
        test_control_panel_hide (NULL, obj, event_info);
        break;
    default:
        break;
    }
}

static void _nf_back_event (void *data, Evas_Object *obj, void *event_info)
{
    if (imf_context) {
        ecore_imf_context_del(imf_context);
        imf_context = NULL;
    }
}

static Evas_Object *_create_imcontrolapi_list (Evas_Object *parent)
{
    int i, num;

    Evas_Object *gl = elm_genlist_add (parent);

    itci.item_style     = "default";
    itci.func.label_get = gli_label_get;
    itci.func.icon_get  = NULL;
    itci.func.state_get = NULL;
    itci.func.del       = NULL;

    num = sizeof (api_list) / sizeof (char *);
    for (i = 0; i < num; i++) {
        elm_genlist_item_append (gl, &itci,
                                 (void *)i/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, test_api/* func */,
                                 (void *)i/* func data */);
    }

    return gl;
}

void imcontrolapi_bt (void *data, Evas_Object *obj, void *event_info)
{
    const char *ctx_id = ecore_imf_context_default_id_get ();
    if (ctx_id != NULL) {
        imf_context = ecore_imf_context_add (ctx_id);
    } else {
        printf ("Cannot create imf context\n");
        return;
    }

    struct appdata *ad = (struct appdata *)data;
    Evas_Object *gl = NULL;

    gl = _create_imcontrolapi_list (ad->naviframe);

    Elm_Object_Item *navi_it = elm_naviframe_item_push (ad->naviframe, _("isfimcontrol api"), NULL, NULL, gl, NULL);

    Evas_Object *back_btn = elm_object_item_content_part_get (navi_it, ELM_NAVIFRAME_ITEM_PREV_BTN);
    evas_object_smart_callback_add (back_btn, "clicked", _nf_back_event, ad);
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
