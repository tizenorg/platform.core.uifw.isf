/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2015 Samsung Electronics Co., Ltd.
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
#include "isf_control.h"

static Ecore_IMF_Context *imf_context = NULL;
static Elm_Genlist_Item_Class itci;
static Evas_Object *win = NULL;

static void test_input_panel_geometry_get (void *data, Evas_Object *obj, void *event_info);
static void test_input_panel_show (void *data, Evas_Object *obj, void *event_info);
static void test_input_panel_hide (void *data, Evas_Object *obj, void *event_info);
static void test_input_panel_imdata_set (void *data, Evas_Object *obj, void *event_info);
static void test_input_panel_imdata_get (void *data, Evas_Object *obj, void *event_info);
static void test_input_panel_layout_set (void *data, Evas_Object *obj, void *event_info);
static void test_input_panel_layout_get (void *data, Evas_Object *obj, void *event_info);
static void test_input_panel_state_get (void *data, Evas_Object *obj, void *event_info);
static void test_input_panel_language_locale_get (void *data, Evas_Object *obj, void *event_info);
static void test_get_active_ise (void *data, Evas_Object *obj, void *event_info);
static void test_get_ise_list (void *data, Evas_Object *obj, void *event_info);
static void test_get_ise_info (void *data, Evas_Object *obj, void *event_info);
static void test_get_initial_ise (void *data, Evas_Object *obj, void *event_info);
static void test_get_ise_count (void *data, Evas_Object *obj, void *event_info);
static void test_reset_default_ise (void *data, Evas_Object *obj, void *event_info);
static void test_show_ime_list (void *data, Evas_Object *obj, void *event_info);
static void test_show_ise_selector (void *data, Evas_Object *obj, void *event_info);
static void test_show_ise_option (void *data, Evas_Object *obj, void *event_info);
static void test_is_ime_enabled (void *data, Evas_Object *obj, void *event_info);
static void test_get_recent_ise_geometry_get (void *data, Evas_Object *obj, void *event_info);

struct _menu_item {
    const char *name;
    void (*func)(void *data, Evas_Object *obj, void *event_info);
};

static struct _menu_item imcontrol_menu_its[] = {
    { "PANEL GEOMETRY GET", test_input_panel_geometry_get },
    { "INPUT PANEL SHOW", test_input_panel_show },
    { "INPUT PANEL HIDE", test_input_panel_hide },
    { "INPUT PANEL IMDATA SET", test_input_panel_imdata_set },
    { "INPUT PANEL IMDATA GET", test_input_panel_imdata_get },
    { "INPUT PANEL LAYOUT SET", test_input_panel_layout_set },
    { "INPUT PANEL LAYOUT GET", test_input_panel_layout_get },
    { "INPUT PANEL STATE GET", test_input_panel_state_get },
    { "LANGUAGE LOCALE GET", test_input_panel_language_locale_get },
    { "GET ACTIVE ISE", test_get_active_ise },
    { "GET ACTIVE ISE INFO", test_get_ise_info },
    { "GET INITIAL ISE", test_get_initial_ise },
    { "GET ISE LIST", test_get_ise_list },
    { "GET ISE COUNT", test_get_ise_count },
    { "RESET DEFAULT ISE", test_reset_default_ise },
    { "SHOW IME LIST", test_show_ime_list },
    { "SHOW ISE SELECTOR", test_show_ise_selector },
    { "SHOW ISE OPTION", test_show_ise_option },
    { "IS IME ENABLED", test_is_ime_enabled },
    { "GET RECENT ISE GEOMETRY", test_get_recent_ise_geometry_get },

    /* do not delete below */
    { NULL, NULL}
};

static void test_input_panel_geometry_get (void *data, Evas_Object *obj, void *event_info)
{
    int x, y, w, h;
    if (imf_context != NULL) {
        ecore_imf_context_input_panel_geometry_get (imf_context, &x, &y, &w, &h);
        LOGD ("x : %d, y : %d, w : %d, h : %d\n", x, y, w, h);
    }
}

static void test_input_panel_show (void *data, Evas_Object *obj, void *event_info)
{
    if (imf_context != NULL) {
        ecore_imf_context_focus_in (imf_context);
        ecore_imf_context_input_panel_show (imf_context);
    }
}

static void test_input_panel_hide (void *data, Evas_Object *obj, void *event_info)
{
    if (imf_context != NULL) {
        ecore_imf_context_focus_out (imf_context);
        ecore_imf_context_input_panel_hide (imf_context);
    }
}

static void test_input_panel_imdata_set (void *data, Evas_Object *obj, void *event_info)
{
    // need ISE to deal with the data
    char buf[256] = "ur imdata";
    if (imf_context != NULL)
        ecore_imf_context_input_panel_imdata_set (imf_context, buf, sizeof (buf));
}

static void test_input_panel_imdata_get (void *data, Evas_Object *obj, void *event_info)
{
    int len = 256;
    char* buf = (char*) calloc (1, len*sizeof (char));
    if (buf != NULL) {
        if (imf_context != NULL) {
            ecore_imf_context_input_panel_imdata_get (imf_context, buf, &len);
            LOGD ("get imdata  %s, and len is %d ...\n", (char *)buf, len);
        }
        free (buf);
    }
}

static void test_input_panel_layout_set (void *data, Evas_Object *obj, void *event_info)
{
    Ecore_IMF_Input_Panel_Layout layout = ECORE_IMF_INPUT_PANEL_LAYOUT_EMAIL;
    if (imf_context != NULL)
        ecore_imf_context_input_panel_layout_set (imf_context, layout);
}

static void test_input_panel_layout_get (void *data, Evas_Object *obj, void *event_info)
{
    Ecore_IMF_Input_Panel_Layout layout;
    if (imf_context != NULL) {
        layout = ecore_imf_context_input_panel_layout_get (imf_context);
        LOGD ("get layout : %d ...\n", (int)layout);
    }
}

static void test_input_panel_state_get (void *data, Evas_Object *obj, void *event_info)
{
    Ecore_IMF_Input_Panel_State state;

    if (imf_context != NULL) {
        state = ecore_imf_context_input_panel_state_get (imf_context);
        LOGD ("ise state : %d \n", (int)state);
    }
}

static void test_input_panel_language_locale_get (void *data, Evas_Object *obj, void *event_info)
{
    char *locale = NULL;

    if (imf_context != NULL) {
        ecore_imf_context_input_panel_language_locale_get (imf_context, &locale);
        LOGD ("locale : %s\n", locale);
    }

    if (locale)
        free (locale);
}

static void test_get_active_ise (void *data, Evas_Object *obj, void *event_info)
{
    char *uuid = NULL;
    int ret = isf_control_get_active_ise (&uuid);
    if (ret > 0 && uuid)
        LOGD ("Get active ISE: %s\n", uuid);
    if (uuid)
        free (uuid);
}

static void test_get_ise_list (void *data, Evas_Object *obj, void *event_info)
{
    char **iselist = NULL;
    int count = isf_control_get_ise_list (&iselist);
    LOGD ("Count : %d\n", count);

    for (int i = 0; i < count; i++) {
        if (iselist[i]) {
            LOGD ("[%d : %s]\n", i, iselist[i]);
            free (iselist[i]);
        }
        else {
            LOGD ("[%d] No data\n", i);
        }
    }
    if (iselist)
        free (iselist);
}

static void test_get_ise_info (void *data, Evas_Object *obj, void *event_info)
{
    char *uuid = NULL;
    int ret = isf_control_get_active_ise (&uuid);
    if (ret > 0 && uuid) {
        char *name      = NULL;
        char *language  = NULL;
        ISE_TYPE_T type = HARDWARE_KEYBOARD_ISE;
        int   option    = 0;
        ret = isf_control_get_ise_info (uuid, &name, &language, &type, &option);
        if (ret == 0 && name && language) {
            LOGD ("Active ISE: uuid[%s], name[%s], language[%s], type[%d], option[%d]\n", uuid, name, language, type, option);
        }
        if (name)
            free (name);
        if (language)
            free (language);
    }
    if (uuid)
        free (uuid);
}

static void test_get_ise_count (void *data, Evas_Object *obj, void *event_info)
{
    LOGD ("S/W keyboard : %d, H/W keyboard : %d\n", isf_control_get_ise_count (SOFTWARE_KEYBOARD_ISE), isf_control_get_ise_count (HARDWARE_KEYBOARD_ISE));
}

static void test_reset_default_ise (void *data, Evas_Object *obj, void *event_info)
{
    int ret = isf_control_set_active_ise_to_default ();
    if (ret == 0)
        LOGD ("Reset default ISE is successful!\n");
    else
        LOGW ("Reset default ISE is failed!!!\n");
}

static void test_show_ime_list (void *data, Evas_Object *obj, void *event_info)
{
    int ret = isf_control_show_ime_list ();
    if (ret == 0)
        LOGD ("Show IME list is successful!\n");
    else
        LOGW ("Show IME list is failed!!!\n");
}

static void test_show_ise_selector (void *data, Evas_Object *obj, void *event_info)
{
    int ret = isf_control_show_ise_selector ();
    if (ret == 0)
        LOGD ("Show ISE selector is successful!\n");
    else
        LOGW ("Show ISE selector is failed!!!\n");
}

static void test_show_ise_option (void *data, Evas_Object *obj, void *event_info)
{
    int ret = isf_control_show_ise_option_window ();
    if (ret == 0)
        LOGD ("Show ISE option window is successful!\n");
    else
        LOGW ("Show ISE option window is failed!!!\n");
}

static void test_is_ime_enabled (void *data, Evas_Object *obj, void *event_info)
{
    char *uuid = NULL;
    bool enabled;
    int ret = isf_control_get_initial_ise (&uuid);
    if (ret > 0 && uuid) {
        LOGD ("Get initial ISE: %s\n", uuid);

        int ret = isf_control_is_ime_enabled (uuid, &enabled);
        if (ret == 0) {
            LOGD ("is_ime_enabled is successful! enabled : %d\n", enabled);
        }
        else {
            LOGW ("is_ime_enabled is failed!\n");
        }
    }
    else
        LOGW ("Failed to get initial ISE\n");

    if (uuid)
        free (uuid);
}

static void test_get_initial_ise (void *data, Evas_Object *obj, void *event_info)
{
    char *uuid = NULL;
    int ret = isf_control_get_initial_ise (&uuid);
    if (ret > 0 && uuid)
        LOGD ("Get initial ISE: %s\n", uuid);
    else
        LOGW ("Failed to get initial ISE\n");

    if (uuid)
        free (uuid);
}

static void test_get_recent_ise_geometry_get (void *data, Evas_Object *obj, void *event_info)
{
    int x, y, w, h;
    if (isf_control_get_recent_ime_geometry (&x, &y, &w, &h) == 0) {
        LOGD ("x : %d, y : %d, width : %d, height : %d\n", x, y, w, h);
    }
    else {
        LOGW ("Failed to get recent ime geometry\n");
    }

    int angle = elm_win_rotation_get (win);

    if (isf_control_get_recent_ime_geometry_with_rotation_angle (angle, &x, &y, &w, &h) == 0) {
        LOGD ("angle : %d, x : %d, y : %d, width : %d, height : %d\n", angle, x, y, w, h);
    }
    else {
        LOGW ("Failed to get recent ime geometry with rotation angle\n");
    }
}

static char *gli_label_get (void *data, Evas_Object *obj, const char *part)
{
    int j = (int)(reinterpret_cast<long>(data));
    return strdup (imcontrol_menu_its[j].name);
}

static void _selected_cb (void *data, Evas_Object *obj, void *event_info)
{
    int j = (int)(reinterpret_cast<long>(data));
    Elm_Object_Item *it = (Elm_Object_Item *)event_info;
    if (it)
        elm_genlist_item_selected_set (it, EINA_FALSE);

    if (imcontrol_menu_its[j].func)
        imcontrol_menu_its[j].func (NULL, obj, event_info);
}

static Eina_Bool _nf_back_event_cb (void *data, Elm_Object_Item *it)
{
    if (imf_context) {
        ecore_imf_context_del (imf_context);
        imf_context = NULL;
    }

    ecore_imf_shutdown ();

    return EINA_TRUE;
}

static Evas_Object *_create_imcontrolapi_list (Evas_Object *parent)
{
    int i = 0;

    Evas_Object *gl = elm_genlist_add (parent);

    itci.item_style     = "default";
    itci.func.text_get  = gli_label_get;
    itci.func.content_get  = NULL;
    itci.func.state_get = NULL;
    itci.func.del       = NULL;

    while (imcontrol_menu_its[i].name != NULL) {
        elm_genlist_item_append (gl, &itci,
                                 (void *)i/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, _selected_cb /* func */,
                                 (void *)i /* func data */);
        i++;
    }

    return gl;
}

static void
win_rotation_changed_cb (void *data, Evas_Object *obj, void *event_info)
{
    struct appdata *ad = (struct appdata *)data;
    int x, y, w, h;
    if (ad == NULL || ad->win_main == NULL)
        return;

    int angle = elm_win_rotation_get (ad->win_main);

    LOGD ("rotate. angle : %d\n", angle);

    if (isf_control_get_recent_ime_geometry (&x, &y, &w, &h) == 0) {
        LOGD ("x : %d, y : %d, width : %d, height : %d\n", x, y, w, h);
    }
    else {
        LOGW ("Failed to get recent ime geometry\n");
    }

    if (isf_control_get_recent_ime_geometry_with_rotation_angle (angle, &x, &y, &w, &h) == 0) {
        LOGD ("angle : %d, x : %d, y : %d, width : %d, height : %d\n", angle, x, y, w, h);
    }
    else {
        LOGW ("Failed to get recent ime geometry with rotation angle\n");
    }
}

void imcontrolapi_bt (void *data, Evas_Object *obj, void *event_info)
{
    ecore_imf_init ();

    const char *ctx_id = ecore_imf_context_default_id_get ();
    if (ctx_id != NULL) {
        imf_context = ecore_imf_context_add (ctx_id);
    } else {
        LOGW ("Cannot create imf context\n");
        ecore_imf_shutdown ();
        return;
    }

    struct appdata *ad = (struct appdata *)data;
    Evas_Object *gl = NULL;

    gl = _create_imcontrolapi_list (ad->naviframe);

    win = ad->win_main;

    Evas_Object *back_btn = create_naviframe_back_button (ad);
    Elm_Object_Item *navi_it = elm_naviframe_item_push (ad->naviframe, _("IM Control"), back_btn, NULL, gl, NULL);
    elm_naviframe_item_pop_cb_set (navi_it, _nf_back_event_cb, ad);

    evas_object_smart_callback_add (ad->win_main, "rotation,changed", win_rotation_changed_cb, ad);
    evas_object_smart_callback_add (ad->win_main, "wm,rotation,changed", win_rotation_changed_cb, ad);
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
