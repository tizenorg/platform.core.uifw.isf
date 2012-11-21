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

#include <stdio.h>
#include <stdlib.h>
#include <Elementary.h>
#include <Ecore_X.h>
#include <privilege-control.h>
#include "isf_demo_efl.h"
#include "isf_imcontrol_efl.h"
#include "isf_layout_efl.h"
#include "isf_event_efl.h"
#include "isf_autocapital_efl.h"
#include "isf_prediction_efl.h"
#include "isf_return_key_type_efl.h"
#include "isf_return_key_disable_efl.h"
#include "isf_imdata_set_efl.h"
#include "isf_focus_movement_efl.h"

#define BASE_THEME_WIDTH 720.0f


static void _quit_cb (void *data, Evas_Object *obj, void *event_info)
{
    elm_exit ();
}

static void _list_click (void *data, Evas_Object *obj, void *event_info)
{
    struct appdata *ad = (struct appdata *)data;
    if (ad == NULL) return;

    Evas_Object *li = ad->li;
    if (li == NULL) return;

    Elm_Object_Item *it = (Elm_Object_Item *)elm_list_selected_item_get (li);

    if (it != NULL)
        elm_list_item_selected_set (it, EINA_FALSE);
}

static void layout_cb (ui_gadget_h ug, enum ug_mode mode, void *priv)
{
    struct appdata *ad = NULL;
    Evas_Object *base = NULL;

    if ( ug == NULL || priv == NULL)
        return;

    ad = (appdata *)priv;

    base = (Evas_Object *)ug_get_layout (ug);
    if (base == NULL)
        return;

    switch (mode) {
    case UG_MODE_FULLVIEW:
        evas_object_size_hint_weight_set (base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_win_resize_object_add (ad->win_main, base);
        evas_object_show (base);
        break;
    case UG_MODE_FRAMEVIEW:
        printf("please set ug mode to UG_MODE_FULLVIEW!\n");
        break;
    default:
        break;
    }
}

static void result_cb (ui_gadget_h ug, service_h s, void *priv)
{
    char *name = NULL;
    service_get_extra_data (s, "name",&name);
    printf("get key [ %s ]\n",name);

    if (strcmp (name, "keyboard-setting-wizard-efl") == 0) {
        char *desp = NULL;
        service_get_extra_data (s, "description",&desp);
        printf("====================\nresult:%s\n====================\n", desp);
        if (desp != NULL)
            free(desp);
    }
    if (name != NULL)
        free (name);
}

static void destroy_cb (ui_gadget_h ug, void *priv)
{
    if (ug == NULL)
        return;

    ug_destroy (ug);
}

static void isfsetting_bt (void *data, Evas_Object *obj, void *event_info)
{
    struct appdata *ad = (struct appdata *)data;
    struct ug_cbs cbs = {0, };

    UG_INIT_EFL (ad->win_main, UG_OPT_INDICATOR_ENABLE);

    cbs.layout_cb  = layout_cb;
    cbs.result_cb  = result_cb;
    cbs.destroy_cb = destroy_cb;
    cbs.priv       = ad;
    ad->ug = ug_create (NULL, "isfsetting-efl",
                        UG_MODE_FULLVIEW,
                        ad->data, &cbs);
    service_destroy (ad->data);
    ad->data = NULL;
}

static void keyboard_setting_wizard_bt (void *data, Evas_Object *obj, void *event_info)
{
    struct appdata *ad = (struct appdata *)data;
    struct ug_cbs cbs = {0, };

    UG_INIT_EFL (ad->win_main, UG_OPT_INDICATOR_ENABLE);

    cbs.layout_cb  = layout_cb;
    cbs.result_cb  = result_cb;
    cbs.destroy_cb = destroy_cb;
    cbs.priv       = ad;
    service_create (&ad->data);
    service_add_extra_data (ad->data, "navi_btn_left", _("Previous"));
    //service_add_extra_data (ad->data, "navi_btn_left", NULL);
    service_add_extra_data (ad->data, "navi_btn_right", _("Next"));
    ad->ug = ug_create (NULL, "keyboard-setting-wizard-efl",
                        UG_MODE_FULLVIEW,
                        ad->data, &cbs);
    service_destroy (ad->data);
    ad->data = NULL;
}

static int create_demo_view (struct appdata *ad)
{
    Evas_Object *li = NULL;

    Evas_Object *l_button = elm_button_add (ad->naviframe);
    elm_object_style_set (l_button, "naviframe/end_btn/default");
    evas_object_smart_callback_add (l_button, "clicked", _quit_cb, NULL);

    // Create list
    ad->li = li = elm_list_add (ad->naviframe);
    elm_list_mode_set (li, ELM_LIST_COMPRESS);
    evas_object_smart_callback_add (ad->li, "selected", _list_click, ad);

    // Test ISF imcontrol API
    elm_list_item_append (li, "ISF IM Control", NULL, NULL, imcontrolapi_bt, ad);

    // test ISF layout
    elm_list_item_append (li, "ISF Layout", NULL, NULL, ise_layout_bt, ad);

    // Test autocapital type
    elm_list_item_append (li, "ISF Autocapital", NULL, NULL, ise_autocapital_bt, ad);

    // Test prediction allow
    elm_list_item_append (li, "ISF Prediction Allow", NULL, NULL, ise_prediction_bt, ad);

    // Test return key type
    elm_list_item_append (li, "ISF Return Key Type", NULL, NULL, ise_return_key_type_bt, ad);

    // Test return key disable
    elm_list_item_append (li, "ISF Return Key Disable", NULL, NULL, ise_return_key_disable_bt, ad);

    // Test imdata setting
    elm_list_item_append (li, "ISF IM Data", NULL, NULL, ise_imdata_set_bt, ad);

    elm_list_item_append (li, "ISF Event", NULL, NULL, isf_event_demo_bt, ad);

    elm_list_item_append (li, "ISF Focus Movement", NULL, NULL, isf_focus_movement_bt, ad);

    elm_list_item_append (li, "ISF Setting", NULL, NULL, isfsetting_bt, ad);
    elm_list_item_append (li, "Keyboard Setting Wizard", NULL, NULL, keyboard_setting_wizard_bt, ad);
    // ISF preedit string and commit string on Label and Entry

    elm_list_go (li);

    elm_naviframe_item_push (ad->naviframe, _("ISF Demo"), l_button, NULL, li, NULL);

    return 0;
}

static int lang_changed (void *data)
{
    struct appdata *ad = (appdata *)data;

    if (ad->layout_main == NULL)
        return 0;

    ug_send_event (UG_EVENT_LANG_CHANGE);
    return 0;
}

static int _rotate_cb (enum appcore_rm m, void *data)
{
    struct appdata *ad = (struct appdata *)data;
    if (ad == NULL || ad->win_main == NULL)
        return 0;

    int r;
    switch (m) {
    case APPCORE_RM_PORTRAIT_NORMAL:
        ug_send_event (UG_EVENT_ROTATE_PORTRAIT);
        r = 0;
        break;
    case APPCORE_RM_PORTRAIT_REVERSE:
        ug_send_event (UG_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN);
        r = 180;
        break;
    case APPCORE_RM_LANDSCAPE_NORMAL:
        ug_send_event (UG_EVENT_ROTATE_LANDSCAPE);
        r = 270;
        break;
    case APPCORE_RM_LANDSCAPE_REVERSE:
        ug_send_event (UG_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN);
        r = 90;
        break;
    default:
        r = -1;
        break;
    }

    if (r >= 0)
        elm_win_rotation_with_resize_set (ad->win_main, r);

    return 0;
}

static void win_del (void *data, Evas_Object *obj, void *event)
{
    elm_exit ();
}

static Evas_Object* create_win (const char *name)
{
    Evas_Object *eo = NULL;
    int w, h;

    eo = elm_win_util_standard_add (name, name);
    if (eo != NULL) {
        evas_object_smart_callback_add (eo, "delete,request",
                                        win_del, NULL);
        ecore_x_window_size_get (ecore_x_window_root_first_get (), &w, &h);
        evas_object_resize (eo, w, h);
    }

    return eo;
}

static void
_vkbd_state_on(void *data, Evas_Object *obj, void *event_info)
{
    printf("[%s] input panel is shown\n", __func__);
}

static void
_vkbd_state_off(void *data, Evas_Object *obj, void *event_info)
{
    printf("[%s] input panel is hidden\n", __func__);
}

static Evas_Object* create_layout_main (Evas_Object *parent)
{
    Evas_Object *layout = elm_layout_add (parent);
    elm_layout_theme_set (layout, "layout", "application", "default");
    evas_object_size_hint_weight_set (layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set (layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show (layout);

    /* Put the layout inside conformant for drawing indicator in app side */
    Evas_Object *conformant = elm_conformant_add (parent);
    evas_object_size_hint_weight_set (conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set (conformant, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_win_resize_object_add (parent, conformant);
    elm_win_conformant_set (parent, EINA_TRUE);
    evas_object_show (conformant);

    evas_object_smart_callback_add(conformant, "virtualkeypad,state,on", _vkbd_state_on, NULL);
    evas_object_smart_callback_add(conformant, "virtualkeypad,state,off", _vkbd_state_off, NULL);

    elm_object_content_set (conformant, layout);

    return layout;
}

static Evas_Object* _create_naviframe_layout (Evas_Object *parent)
{
    Evas_Object *naviframe = elm_naviframe_add (parent);
    elm_object_part_content_set (parent, "elm.swallow.content", naviframe);

    evas_object_show (naviframe);

    return naviframe;
}

static Eina_Bool _keydown_event (void *data, int type, void *event)
{
    Ecore_Event_Key *ev = (Ecore_Event_Key *)event;
    struct appdata *ad = (struct appdata *)data;
    Elm_Object_Item *top_it, *bottom_it;
    if (ad == NULL || ev == NULL) return ECORE_CALLBACK_RENEW;

    printf ("[ecore key down] keyname : '%s', key : '%s', string : '%s', compose : '%s'\n", ev->keyname, ev->key, ev->string, ev->compose);

    if (strcmp (ev->keyname, "XF86Stop") == 0) {
        if (ug_send_key_event (UG_KEY_EVENT_END) == -1) {
            top_it = elm_naviframe_top_item_get (ad->naviframe);
            bottom_it = elm_naviframe_top_item_get (ad->naviframe);
            if (top_it && bottom_it && (elm_object_item_content_get (top_it) == elm_object_item_content_get (bottom_it))) {
                elm_exit ();
            } else {
                elm_naviframe_item_pop (ad->naviframe);
            }
        }
    }
    return ECORE_CALLBACK_RENEW;
}

static Eina_Bool _keyup_event (void *data, int type, void *event)
{
    Ecore_Event_Key *ev = (Ecore_Event_Key *)event;

    printf ("[ecore key up] keyname : '%s', key : '%s', string : '%s', compose : '%s'\n", ev->keyname, ev->key, ev->string, ev->compose);

    return ECORE_CALLBACK_RENEW;
}

static int app_create (void *data)
{
    struct appdata *ad = (struct appdata *)data;

    appcore_measure_start ();

    ad->win_main = create_win ("isf-demo-efl");
    evas_object_show (ad->win_main);

    ad->evas = evas_object_evas_get (ad->win_main);
    /* get width and height of main window */
    evas_object_geometry_get (ad->win_main, NULL, NULL, &ad->root_w, &ad->root_h);

    if (ad->root_w >= 0) {
        elm_config_scale_set (ad->root_w / BASE_THEME_WIDTH);
    }

    ad->layout_main = create_layout_main (ad->win_main);

    // Indicator
    elm_win_indicator_mode_set (ad->win_main, ELM_WIN_INDICATOR_SHOW);

    // Navigation Bar
    ad->naviframe = _create_naviframe_layout (ad->layout_main);

    //init the content in layout_main.
    create_demo_view (ad);

    lang_changed (ad);

    evas_object_show (ad->win_main);

    /* add system event callback */
    appcore_set_event_callback (APPCORE_EVENT_LANG_CHANGE,
                                lang_changed, ad);

    ecore_event_handler_add (ECORE_EVENT_KEY_DOWN, _keydown_event, ad);
    ecore_event_handler_add (ECORE_EVENT_KEY_UP, _keyup_event, ad);

    appcore_set_rotation_cb (_rotate_cb, ad);

    appcore_measure_time ();

    return 0;
}

static int app_exit (void *data)
{
    struct appdata *ad = (struct appdata *)data;
    if (ad->li != NULL)
        evas_object_del (ad->li);

    if (ad->ev_li != NULL)
        evas_object_del (ad->ev_li);

    if (ad->layout_main != NULL)
        evas_object_del (ad->layout_main);

    if (ad->win_main != NULL)
        evas_object_del (ad->win_main);

    return 0;
}

static int app_pause(void *data)
{
    return 0;
}

static int app_resume (void *data)
{
    return 0;
}

int main (int argc, char *argv[])
{
    set_app_privilege ("isf", NULL, NULL);

    struct appdata ad;
    struct appcore_ops ops;

    ops.create    = app_create;
    ops.terminate = app_exit;
    ops.pause     = app_pause;
    ops.resume    = app_resume;
    ops.reset     = NULL;

    memset (&ad, 0x0, sizeof (struct appdata));
    ops.data = &ad;
    return appcore_efl_main ("isf-demo-efl", &argc, &argv, &ops);
}

static void _focused_cb(void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *ly = (Evas_Object *)data;

    elm_object_signal_emit (ly, "elm,state,guidetext,hide", "elm");
}

static void _unfocused_cb (void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *ly = (Evas_Object *)data;

    if (elm_entry_is_empty (obj))
        elm_object_signal_emit (ly, "elm,state,guidetext,show", "elm");
}

// Utility functions
Evas_Object *create_ef (Evas_Object *parent, const char *label, const char *guide_text)
{
    Evas_Object *ef = NULL;
    Evas_Object *en = NULL;

    ef = elm_layout_add (parent);
    elm_layout_theme_set (ef, "layout", "editfield", "title");

    en = elm_entry_add (parent);
    elm_object_part_content_set (ef, "elm.swallow.content", en);

    elm_object_part_text_set (ef, "elm.text", label);
    elm_object_part_text_set (ef, "elm.guidetext", guide_text);
    evas_object_size_hint_weight_set (ef, EVAS_HINT_EXPAND, 0);
    evas_object_size_hint_align_set (ef, EVAS_HINT_FILL, 0);
    evas_object_smart_callback_add (en, "focused", _focused_cb, ef);
    evas_object_smart_callback_add (en, "unfocused", _unfocused_cb, ef);
    evas_object_show (ef);

    return ef;
}

void add_layout_to_naviframe (void *data, Evas_Object *lay_in, const char *title)
{
    struct appdata *ad = (struct appdata *) data;

    Evas_Object *scroller = elm_scroller_add (ad->naviframe);
    elm_scroller_bounce_set (scroller, EINA_FALSE, EINA_TRUE);
    evas_object_show (scroller);

    elm_object_content_set (scroller, lay_in);
    elm_naviframe_item_push (ad->naviframe, title, NULL, NULL, scroller, NULL);
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
