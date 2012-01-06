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

#ifndef UG_WIZARD_MODULE_API
#define UG_WIZARD_MODULE_API __attribute__ ((visibility("default")))
#endif

#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_HELPER_MODULE

#include <stdio.h>
#include <Elementary.h>
#include <Ecore_IMF.h>
#include "isf_control.h"
#include <Ecore_X.h>
#include <glib.h>
#include <glib-object.h>
#include <vconf-keys.h>
#include <vconf.h>
#include <ui-gadget-module.h>
#include <ui-gadget.h>
#include <dlog.h>
#include "scim.h"
#include "scim_stl_map.h"
#include "isf_setting_wizard.h"
#include "../efl_panel/isf_panel_utility.h"


using namespace scim;


#define WIZARD_PACKAGE                             "keyboard-setting-wizard-efl"
#define WIZARD_LOCALEDIR                           "/opt/ug/res/locale"
#define T_(s)                                      dgettext(WIZARD_PACKAGE, s)
#define LOG_TAG                                    "isfsettingwizard"

static Elm_Genlist_Item_Class itc1,itcSeparator;
static int mark = 0;


static Ecore_IMF_Context *imf_context = NULL;

static Evas_Object *sw_radio_grp   = NULL;      //sw view raido group
static std::vector<String> sw_iselist;

static ConfigPointer _config;

static char ise_bak[256] = {'\0'};
static char _active_ise_name[256] = {'\0'};


extern std::vector <String>  _names;
extern std::vector <String>  _uuids;
extern std::vector <String>  _module_names;
extern std::vector <String>  _langs;

static String uuid_to_name(String uuid)
{
    String tmpName("");
    for(unsigned int i = 0;i<_uuids.size();i++)
    {
        if (strcmp(uuid.c_str(),_uuids[i].c_str())== 0) {
            tmpName = _names[i];
            break;
        }
    }
    return tmpName;
}

static Evas_Object *_create_bg(Evas_Object *win)
{
    Evas_Object *bg = elm_bg_add(win);
    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(bg);
    return bg;
}

static Evas_Object *create_fullview (Evas_Object *parent, struct ug_data *ugd)
{
    Evas_Object *bg = _create_bg(parent);
    Evas_Object *layout_main = NULL;

    layout_main = elm_layout_add (parent);

    if (layout_main == NULL)
        return NULL;

    elm_layout_theme_set (layout_main, "layout","application","default");
    elm_object_style_set(bg, "group_list");
    elm_layout_content_set (layout_main, "elm.swallow.bg", bg);

    return layout_main;
}

static Evas_Object *create_frameview (Evas_Object *parent, struct ug_data *ugd)
{
    Evas_Object *bg = _create_bg(parent);
    Evas_Object *layout_main = elm_layout_add (parent);
    if (layout_main == NULL)
        return NULL;
    elm_layout_theme_set (layout_main, "layout", "application", "default");
    elm_object_style_set(bg, "group_list");
    elm_layout_content_set (layout_main, "elm.swallow.bg", bg);

    return layout_main;
}

static void back_cb (void *data, Evas_Object *obj, void *event_info)
{
    if (data == NULL)
        return;

    struct ug_data *ugd = (struct ug_data *)data;
    ug_destroy_me (ugd->ug);
}

static Evas_Object* _create_naviframe_layout (Evas_Object* parent)
{
    Evas_Object *naviframe = elm_naviframe_add (parent);
    elm_layout_content_set (parent, "elm.swallow.content", naviframe);
    evas_object_show (naviframe);

    return naviframe;
}

static bool in_exit = false;
static void sw_keyboard_selection_view_back_cb (void *data, Evas_Object *obj, void *event_info)
{
    if (in_exit) {
        LOGD("do nothing ,exit!\n");
        return;
    }
    in_exit = true;

    if (data == NULL)
        return;

    struct ug_data *ugd = NULL;
    ugd = (ug_data *)data;

    if (strcmp (ise_bak, _active_ise_name) != 0) {
    //if _active_ise_name is changed , active _active_ise_name.
    //find the uuid of the active
        String uuid;
        for (unsigned int i = 0; i < _names.size (); i++) {
            if (strcmp (_names[i].c_str (), _active_ise_name) == 0)
                uuid = _uuids[i];
        }
        isf_control_set_active_ise_by_uuid ( uuid.c_str ());

        snprintf (ise_bak, sizeof (ise_bak), "%s", _active_ise_name);
    }

    bundle *b = NULL;
    b = bundle_create();
    bundle_add(b, "name", "keyboard-setting-wizard-efl");
    bundle_add(b, "description", "previous clicked");
    ug_send_result(ugd->ug, b);
    bundle_free(b);

    back_cb(data,obj,event_info);

    LOGD("End of %s", __func__);
}

static void sw_keyboard_selection_view_set_cb (void *data, Evas_Object *obj, void *event_info)
{
    if (in_exit)
        return;
    in_exit = true;

    if (data == NULL)
        return;

    struct ug_data *ugd = (struct ug_data *)data;

    if (strcmp (ise_bak, _active_ise_name) != 0) {
        //if _active_ise_name is changed , active _active_ise_name.
        //find the uuid of the active
        String uuid;
        for (unsigned int i = 0; i < _names.size (); i++) {
            if (strcmp (_names[i].c_str (), _active_ise_name) == 0)
                uuid = _uuids[i];
        }
        isf_control_set_active_ise_by_uuid ( uuid.c_str ());

        snprintf (ise_bak, sizeof (ise_bak), "%s", _active_ise_name);
        LOGD("Set active ISE : %s", ise_bak);
    }

    bundle *b = NULL;
    b = bundle_create();
    bundle_add(b, "name", "keyboard-setting-wizard-efl");
    bundle_add(b, "description", "next clicked");
    ug_send_result(ugd->ug, b);
    bundle_free(b);

    back_cb(data,obj,event_info);

    LOGD("End of %s", __func__);
//   call next ug
}

static void _gl_sel(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Genlist_Item *item = (Elm_Genlist_Item *)event_info;
    elm_genlist_item_selected_set(item, 0);
    mark = (int) (data);
    snprintf (_active_ise_name, sizeof (_active_ise_name), "%s", sw_iselist[mark].c_str ());
    elm_genlist_item_update(item);
    return;
}

static void _sw_radio_cb (void *data, Evas_Object *obj, void *event_info)
{
    int index = GPOINTER_TO_INT(data);
    elm_radio_value_set (sw_radio_grp, index);
    snprintf (_active_ise_name, sizeof (_active_ise_name), "%s", sw_iselist[index].c_str ());
}

static char *_gl_label_get(void *data, Evas_Object *obj, const char *part)
{
    int index = (int)(data);
    if (!strcmp(part, "elm.text")) {
        return strdup(sw_iselist[index].c_str());
    }
    return NULL;
}

static Evas_Object *_gl_icon_get(void *data, Evas_Object *obj, const char *part)
{
    if (!strcmp(part, "elm.icon")) {

        int index = (int)(data);
        Evas_Object *radio  = elm_radio_add (obj);
        elm_radio_state_value_set (radio, index);
        if (sw_radio_grp == NULL)
            sw_radio_grp = elm_radio_add(obj);
        elm_radio_group_add (radio, sw_radio_grp);
        evas_object_show (radio);
        evas_object_smart_callback_add (radio, "changed", _sw_radio_cb, (void *) (index));
        if (mark == index) {
            elm_radio_value_set (sw_radio_grp, mark);
        }
        return radio;
    }
    return NULL;
}

static Eina_Bool _gl_state_get(void *data, Evas_Object *obj, const char *part)
{
    return EINA_FALSE;
}

static void _gl_del(void *data, Evas_Object *obj)
{
    return;
}

static Evas_Object *isf_setting_main_view_tizen(ug_data * ugd)
{
   String tmpStr = _config->read(SCIM_CONFIG_DEFAULT_HELPER_ISE,String("b70bf6cc-ff77-47dc-a137-60acc32d1e0c"));
   snprintf (_active_ise_name, sizeof (_active_ise_name), "%s", (uuid_to_name(tmpStr)).c_str());
   snprintf (ise_bak, sizeof (ise_bak), "%s", _active_ise_name);
   LOGD("Default ISE Name : %s", ise_bak);

    ugd->naviframe = _create_naviframe_layout (ugd->layout_main);
    const char *navi_btn_l_lable = bundle_get_val(ugd->data, "navi_btn_left");
    const char *navi_btn_r_lable = bundle_get_val(ugd->data, "navi_btn_right");

    if (sw_radio_grp != NULL)
    {
        evas_object_del(sw_radio_grp);
        sw_radio_grp = NULL;
    }

    Evas_Object *genlist = elm_genlist_add(ugd->naviframe);
    evas_object_show(genlist);

    Elm_Object_Item *nf_it;
    Evas_Object *cbar = elm_controlbar_add(ugd->naviframe);
    if (cbar == NULL) return NULL;
    elm_object_style_set(cbar, "naviframe");

    if (navi_btn_l_lable!= NULL) {
        elm_controlbar_tool_item_append(cbar, NULL, navi_btn_l_lable, sw_keyboard_selection_view_back_cb, ugd);
        elm_controlbar_tool_item_append(cbar, NULL, navi_btn_r_lable, sw_keyboard_selection_view_set_cb, ugd);

        nf_it = elm_naviframe_item_push(ugd->naviframe, T_("Keyboard"), NULL, NULL, genlist, NULL);
        elm_object_item_part_content_set(nf_it, ELM_NAVIFRAME_ITEM_CONTROLBAR, cbar);
    }
    else {
        elm_controlbar_tool_item_append(cbar, NULL, navi_btn_r_lable, sw_keyboard_selection_view_set_cb, ugd);
        nf_it = elm_naviframe_item_push(ugd->naviframe, T_("Keyboard"), NULL, NULL, genlist, NULL);
        elm_object_item_part_content_set(nf_it, ELM_NAVIFRAME_ITEM_CONTROLBAR, cbar);
    }

    unsigned int i = 0;

    sw_iselist.clear();
    std::vector<String> selected_langs, all_langs;

    isf_get_all_languages (all_langs);
    isf_get_helper_names_in_languages (all_langs, sw_iselist);
    std::sort (sw_iselist.begin (), sw_iselist.end ());

    if (sw_iselist.size () > 0) {
        // Set item class for dialogue group seperator
        itcSeparator.item_style = "dialogue/separator/21/with_line";
        itcSeparator.func.label_get = NULL;
        itcSeparator.func.icon_get = NULL;
        itcSeparator.func.state_get = NULL;
        itcSeparator.func.del = NULL;

        //separator
        Elm_Genlist_Item *item;
        item = elm_genlist_item_append(
                    genlist,                // genlist object
                    &itcSeparator,          // item class
                    NULL,                   // data
                    NULL,
                    ELM_GENLIST_ITEM_NONE,
                    NULL,
                    NULL);
        elm_genlist_item_display_only_set(item, EINA_TRUE);
    }

    for (i = 0; i < sw_iselist.size (); i++) {
        if (strcmp (_active_ise_name, sw_iselist[i].c_str ()) == 0) {
            mark = i;
            break;
        }
    }

    //set item class for 1text.1icon(text+radiobutton)
    itc1.item_style = "dialogue/1text.1icon.2";
    itc1.func.label_get = _gl_label_get;
    itc1.func.icon_get = _gl_icon_get;
    itc1.func.state_get = _gl_state_get;
    itc1.func.del = _gl_del;
    for (i = 0;i < sw_iselist.size();i++) {
        elm_genlist_item_append(genlist, &itc1,
                    (void *)(i), NULL, ELM_GENLIST_ITEM_NONE, _gl_sel,
                    (void *)(i));
    }

    return ugd->naviframe;
}

ConfigPointer isf_imf_context_get_config(void);
static void *on_create (struct ui_gadget *ug, enum ug_mode mode, bundle *data, void *priv)
{
    Evas_Object *parent = NULL;
    Evas_Object *content = NULL;

    if ( ug == NULL || priv == NULL)
        return NULL;

    bindtextdomain (WIZARD_PACKAGE, WIZARD_LOCALEDIR);

    struct ug_data *ugd = (struct ug_data *)priv;
    ugd->ug = ug;
    ugd->data = data;
    parent = (Evas_Object *) ug_get_parent_layout (ug);
    if (parent == NULL)
        return NULL;
    //-------------------------- ise infomation ----------------------------

    const char *ctx_id = ecore_imf_context_default_id_get ();
    if (ctx_id != NULL) {
        imf_context = ecore_imf_context_add (ctx_id);
    }

    _config = isf_imf_context_get_config ();
    if (_config.null ()) {
        std::cerr << "Create dummy config!!!\n";
        _config = new DummyConfig ();
    }

    if (_config.null ()) {
        std::cerr << "Can not create Config Object!\n";
    }

    //only helper ISEs will be needed in isfsetting according to phone requirement.
    isf_load_ise_information (HELPER_ONLY, _config);
    // Request ISF to update ISE list, below codes are very important, dont remove
    char **iselist = NULL;
    int count = isf_control_get_iselist (&iselist);
    for (unsigned int i = 0; i < (unsigned int)count; i++) {
        SCIM_DEBUG_MAIN (3) << " [" << i << " : " << iselist[i] << "] \n";
        if (iselist[i] != NULL)
            delete []  (iselist[i]);
    }

    if (iselist!=NULL)
        free(iselist);
    //-------------------------- ise infomation ----------------------------

    //construct the UI part of the isfsetting module
    if (mode == UG_MODE_FULLVIEW)
        ugd->layout_main = create_fullview (parent, ugd);
    else
        ugd->layout_main = create_frameview (parent, ugd);

    if (ugd->layout_main != NULL) {
        content = isf_setting_main_view_tizen(ugd);
        elm_layout_content_set (ugd->layout_main, "elm.swallow.content", content);
    }
    return (void *)ugd->layout_main;
}

static void on_start (struct ui_gadget *ug, bundle *data, void *priv)
{
}

static void on_pause (struct ui_gadget *ug, bundle *data, void *priv)
{

}

static void on_resume (struct ui_gadget *ug, bundle *data, void *priv)
{

}

static void on_destroy (struct ui_gadget *ug, bundle *data, void *priv)
{
    if ( ug == NULL|| priv == NULL)
        return;

    if (imf_context != NULL) {
        ecore_imf_context_del(imf_context);
        imf_context = NULL;
    }

    struct ug_data *ugd = (struct ug_data *) priv;

    if (ugd->naviframe != NULL) {
        evas_object_del (ugd->naviframe);
        ugd->naviframe = NULL;
    }

    if (ugd->layout_main != NULL) {
        evas_object_del (ugd->layout_main);
        ugd->layout_main = NULL;
    }

    if (!_config.null ()) {
        _config->flush ();
        _config.reset ();
    }
}

static void on_message (struct ui_gadget *ug, bundle *msg, bundle *data, void *priv)
{
}

static void on_event (struct ui_gadget *ug, enum ug_event event, bundle *data, void *priv)
{
    switch (event) {
    case UG_EVENT_LOW_MEMORY:
        break;
    case UG_EVENT_LOW_BATTERY:
        break;
    case UG_EVENT_LANG_CHANGE:
        break;
    case UG_EVENT_ROTATE_PORTRAIT:
        break;
    case UG_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN:
        break;
    case UG_EVENT_ROTATE_LANDSCAPE:
        break;
    case UG_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN:
        break;
    default:
        break;
    }
}

static void on_key_event(struct ui_gadget *ug, enum ug_key_event event, bundle *data, void *priv)
{
    if (ug == NULL)
        return;

    switch (event) {
        case UG_KEY_EVENT_END:
            ug_destroy_me(ug);
            break;
        default:
            break;
    }
}

#ifdef __cplusplus
extern "C"
{
#endif
    UG_WIZARD_MODULE_API int UG_MODULE_INIT (struct ug_module_ops *ops) {
        if (ops == NULL)
            return -1;

        struct ug_data *ugd = (ug_data*)calloc (1, sizeof (struct ug_data));
        if (ugd == NULL)
            return -1;

        ops->create  = on_create;
        ops->start   = on_start;
        ops->pause   = on_pause;
        ops->resume  = on_resume;
        ops->destroy = on_destroy;
        ops->message = on_message;
        ops->event   = on_event;
        ops->key_event = on_key_event;
        ops->priv    = ugd;
        ops->opt     = UG_OPT_INDICATOR_PORTRAIT_ONLY;

        return 0;
    }

    UG_WIZARD_MODULE_API void UG_MODULE_EXIT (struct ug_module_ops *ops) {
        if (ops == NULL)
            return;

        struct ug_data *ugd = (struct ug_data *)(ops->priv);
        if (ugd != NULL)
            free (ugd);
    }

#ifdef __cplusplus
}
#endif
/*
vi:ts=4:ai:nowrap:expandtab
*/

