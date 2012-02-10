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

#include <stdio.h>
#include <stdlib.h>
#include <Elementary.h>
#include <ui-gadget.h>
#include <Ecore_X.h>
#include "isf_demo_efl.h"
#include "isf_imcontrol_efl.h"
#include "isf_layout_efl.h"
#include "isf_event_efl.h"
#include "isf_autocapital_efl.h"
#include "isf_prediction_efl.h"

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

    Elm_List_Item *it = (Elm_List_Item *)elm_list_selected_item_get (li);

    if (it != NULL)
        elm_list_item_selected_set (it, EINA_FALSE);
}

static void layout_cb (struct ui_gadget *ug, enum ug_mode mode, void *priv)
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

static void result_cb (struct ui_gadget *ug, bundle *result, void *priv)
{
    if (!result) return;

    const char *name = bundle_get_val(result, "name");
    printf("get key [ %s ]\n",name);

    if (strcmp(name, "keyboard-setting-wizard-efl") == 0) {
        const char *desp = bundle_get_val(result,"description");
        printf("====================\nresult:%s\n====================\n",desp);
    }
}

static void destroy_cb (struct ui_gadget *ug, void *priv)
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
    bundle_free (ad->data);
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
    ad->data = bundle_create();
    bundle_add(ad->data, "navi_btn_left", _("Previous"));
    //bundle_add(ad->data, "navi_btn_left", NULL);
    bundle_add(ad->data, "navi_btn_right", _("Next"));
    ad->ug = ug_create (NULL, "keyboard-setting-wizard-efl",
                        UG_MODE_FULLVIEW,
                        ad->data, &cbs);
    bundle_free (ad->data);
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
    elm_list_item_append (li, "ISF imcontrol API", NULL, NULL, imcontrolapi_bt, ad);

    // test ISF layout
    elm_list_item_append (li, "ISF Layout", NULL, NULL, ise_layout_bt, ad);

    // Test autocapital type
    elm_list_item_append (li, "ISF Autocapital", NULL, NULL, ise_autocapital_bt, ad);

    // Test prediction allow
    elm_list_item_append (li, "ISF Prediction allow", NULL, NULL, ise_prediction_bt, ad);

    elm_list_item_append (li, "ISF Event Demo", NULL, NULL, isf_event_demo_bt, ad);

    /*
    ISF language selection
    ISE selection
    */
    elm_list_item_append (li, "ISF Setting", NULL, NULL, isfsetting_bt, ad);
    elm_list_item_append (li, "keyboard Setting wizard", NULL, NULL, keyboard_setting_wizard_bt, ad);
    // ISF preedit string and commit string on Label and Entry

    elm_list_go (li);

    elm_naviframe_item_push (ad->naviframe, _("isf demo"), l_button, NULL, li, NULL);

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

    eo = elm_win_add (NULL, name, ELM_WIN_BASIC);
    if (eo != NULL) {
        elm_win_title_set (eo, name);
        elm_win_borderless_set (eo, EINA_TRUE);
        evas_object_smart_callback_add (eo, "delete,request",
                                        win_del, NULL);
        ecore_x_window_size_get (ecore_x_window_root_first_get (), &w, &h);
        evas_object_resize (eo, w, h);
    }

    return eo;
}

static Evas_Object* create_layout_main (Evas_Object* parent)
{
    Evas_Object *layout = elm_layout_add (parent);
    elm_layout_theme_set (layout, "layout", "application", "default");
    evas_object_size_hint_weight_set (layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add (parent, layout);

    evas_object_show (layout);

    return layout;
}

static Evas_Object* _create_naviframe_layout (Evas_Object* parent)
{
    Evas_Object *naviframe = elm_naviframe_add (parent);
    elm_layout_content_set (parent, "elm.swallow.content", naviframe);

    evas_object_show (naviframe);

    return naviframe;
}

static Evas_Object* create_bg(Evas_Object *win)
{
    Evas_Object *bg = elm_bg_add(win);
    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(win, bg);
    evas_object_show(bg);

    return bg;
}

static Eina_Bool _keydown_event(void *data, int type, void *event)
{
    Ecore_Event_Key *ev = (Ecore_Event_Key *)event;
    struct appdata *ad = (struct appdata *)data;
    Elm_Object_Item *top_it, *bottom_it;
    if (ad == NULL || ev == NULL) return ECORE_CALLBACK_RENEW;

    if (strcmp(ev->keyname, "XF86Stop") == 0) {
        if (ug_send_key_event(UG_KEY_EVENT_END) == -1) {
            top_it = elm_naviframe_top_item_get(ad->naviframe);
            bottom_it = elm_naviframe_top_item_get(ad->naviframe);
            if (top_it && bottom_it && (elm_object_item_content_get(top_it) == elm_object_item_content_get(bottom_it))) {
                elm_exit();
            }
            else {
                elm_naviframe_item_pop(ad->naviframe);
            }
        }
    }
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
        elm_scale_set(ad->root_w / BASE_THEME_WIDTH );
    }

    ad->bg = create_bg(ad->win_main);

    ad->layout_main = create_layout_main (ad->win_main);

    // Indicator
    elm_win_indicator_state_set (ad->win_main, EINA_TRUE);

    // Navigation Bar
    ad->naviframe = _create_naviframe_layout (ad->layout_main);

    //init the content in layout_main.
    create_demo_view (ad);

    lang_changed (ad);

    evas_object_show (ad->win_main);

    /* add system event callback */
    appcore_set_event_callback (APPCORE_EVENT_LANG_CHANGE,
                                lang_changed, ad);

    ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, _keydown_event, ad);

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

static int app_reset (bundle *b, void *data)
{
    struct appdata *ad = (struct appdata *)data;

    if ( (ad != NULL) && (ad->win_main != NULL))
        elm_win_activate (ad->win_main);

    return 0;
}

int main (int argc, char *argv[])
{
    struct appdata ad;
    struct appcore_ops ops;

    ops.create    = app_create;
    ops.terminate = app_exit;
    ops.pause     = app_pause;
    ops.resume    = app_resume;
    ops.reset     = app_reset;

    memset (&ad, 0x0, sizeof (struct appdata));
    ops.data = &ad;
    return appcore_efl_main ("isf-demo-efl", &argc, &argv, &ops);
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
