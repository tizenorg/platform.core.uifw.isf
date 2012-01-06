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

#ifndef UG_MODULE_API
#define UG_MODULE_API __attribute__ ((visibility("default")))
#endif

#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_HELPER_MODULE

#define SINGLE_SELECTION

#include <stdio.h>
#include <Elementary.h>
#include <Ecore_IMF.h>
#include <Ecore_X.h>
#include <glib.h>
#include <glib-object.h>
#include <vconf-keys.h>
#include <vconf.h>
#include <ui-gadget-module.h>
#include <ui-gadget.h>
#include "scim.h"
#include "scim_stl_map.h"
#include "isf_setting_efl.h"
#include "../efl_panel/isf_panel_utility.h"
#include "scim_setup_module_efl.h"
#include "isf_control.h"


using namespace scim;


#define SETTING_AUTO_CAPITALIZATION               "isfsetting/auto_capitalization"
#define SETTING_SW_KEYBOARD_CONNECTION            "isfsetting/sw_keyboard_connection"

#define _EDJ(x)                                   elm_layout_edje_get(x)
#define ISF_SETTING_EDJ                           (SCIM_DATADIR "/isfsetting.edj")
#define PADDING_X                                 25

#define SETTING_PACKAGE                           "isfsetting-efl"
#define SETTING_LOCALEDIR                         "/opt/ug/res/locale"
#define _T(s)                                     dgettext(SETTING_PACKAGE, s)


enum {
    F_CONNECTION_AUTO = 0,
    F_CONNECTION_ALWAYS_USED,
    F_CONNECTION_END,
};

enum {
    TYPE_PHONEPAD = 0,
    TYPE_QWERTY,
    TYPE_KEYBOARD_END,
};

enum
{
    AUTO_CAPITALIZATION_ITEM =0,
    SW_KEYBOARD_GROUP_TITLE_ITEM,
    SW_KEYBOARD_SEL_ITEM,
    SW_ISE_OPTION_ITEM,
    HW_KEYBOARD_GROUP_TITLE_ITEM,
    HW_KEYBOARD_SEL_ITEM,
    HW_ISE_OPTION_ITEM,
    ITEM_TOTAL_COUNT
};

struct ItemData
{
    char *text;
    char *sub_text;
    int mode;
};

static struct ug_data *common_ugd ;
static ItemData *p_items[ITEM_TOTAL_COUNT] ;
static Elm_Genlist_Item_Class itc1, itc2, itc3, itc4 ,itc5, itcTitle, itcSeparator;
static int mark = 0;
static int hw_mark = 0;
static Ecore_Event_Handler *_prop_change_handler = NULL;
static Ecore_X_Atom prop_x_ext_keyboard_exist = 0;
static Ecore_X_Window _rootwin;
static unsigned int hw_kbd_num = 0;

#ifndef SINGLE_SELECTION
static Evas_Object *default_ise_item = NULL;
static std::vector<String> enable_uuid_list;
static String _default_ise_uuid;
#endif

static bool is_hw_connected          = false;
static Evas_Object *hw_radio_grp     = NULL;
static std::vector<String> hw_iselist;

static char hw_ise_bak[256]          = {'\0'};
static char _active_hw_ise_name[256] = {'\0'};

static Ecore_IMF_Context *imf_context = NULL;
static SetupModule *mdl              = NULL;
static Evas_Object *sw_radio_grp     = NULL;      //test view raido group
static std::vector<String> sw_iselist;

static Eina_Bool f_auto_capitalization = EINA_FALSE;

static ConfigPointer _config;
static char ise_bak[256] = {'\0'};
static char _active_ise_name[256] = {'\0'};

extern std::vector <String>  _names;
extern std::vector <String>  _uuids;
extern std::vector <String>  _module_names;
extern std::vector <String>  _langs;


static Evas_Object *_gl_icon_get(void *data, Evas_Object *obj, const char *part);
static char *_gl_label_get(void *data, Evas_Object *obj, const char *part);
static Eina_Bool _gl_state_get(void *data, Evas_Object *obj, const char *part);
static void _gl_del(void *data, Evas_Object *obj);
static void _gl_sel(void *data, Evas_Object *obj, void *event_info);
static void _gl_sw_ise_sel(void *data, Evas_Object *obj, void *event_info);
static void _gl_hw_ise_sel(void *data, Evas_Object *obj, void *event_info);
static void _gl_keyboard_sel(void *data, Evas_Object *obj, void *event_info);
static char *_gl_exp_sw_label_get(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_exp_sw_icon_get(void *data, Evas_Object *obj, const char *part);
static void sw_keyboard_selection_view_tizen(ug_data * ugd);
static void hw_keyboard_selection_view_tizen(ug_data * ugd);
static void show_ise_option_module(ug_data *ugd , const char *isename);
static void _hw_radio_cb (void *data, Evas_Object *obj, void *event_info);

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

    elm_layout_theme_set (layout_main, "layout", "application", "default");
    elm_object_style_set(bg, "group_list");
    elm_object_part_content_set (layout_main, "elm.swallow.bg", bg);

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
    elm_object_part_content_set (layout_main, "elm.swallow.bg", bg);
    return layout_main;
}

static bool in_exit = false;
static void back_cb (void *data, Evas_Object *obj, void *event_info)
{
    if (in_exit)
        return;
    in_exit = true;

    if (data == NULL)
        return;
   struct ug_data *ugd = (struct ug_data *)data;
    ug_destroy_me (ugd->ug);
}

static Evas_Object* _create_naviframe_layout (Evas_Object* parent)
{
    Evas_Object *naviframe = elm_naviframe_add (parent);
    elm_object_part_content_set (parent, "elm.swallow.content", naviframe);
    evas_object_show (naviframe);

    return naviframe;
}

#ifndef SINGLE_SELECTION
static void _ise_onoff_cb (void *data, Evas_Object *obj, void *event_info)
{
    int i = (int)data;
    if (elm_check_state_get (obj) == EINA_FALSE) {
        // remove  the uuid from enable_uuid_list
        std::vector<String>::iterator it;
        it = std::find (enable_uuid_list.begin (), enable_uuid_list.end (), _uuids[i]);
        enable_uuid_list.erase (it);
    } else {
        //add the uuid into the disable_uuid_list
        enable_uuid_list.push_back (_uuids[i]);
    }

    //check if the active keyboard num == 0?
    int num = enable_uuid_list.size ();
    if (num == 0) {
        //search the DEFAULT_ISE  onoff check and set it on
        elm_check_state_set (default_ise_item, EINA_TRUE);
        enable_uuid_list.push_back (_default_ise_uuid);
    }
}
#endif

static void set_autocap_mode()
{
    if (vconf_set_bool (VCONFKEY_SETAPPL_AUTOCAPITAL_ALLOW_BOOL, f_auto_capitalization) == -1)
        printf("Failed to set vconf autocapital\n");
}

static void _onoff_cb (void *data, Evas_Object *obj, void *event_info)
{
    int index = (int)data;

    if (index == AUTO_CAPITALIZATION_ITEM) {
        if (elm_check_state_get (obj) == EINA_TRUE)
            f_auto_capitalization = EINA_TRUE;
        else
            f_auto_capitalization = EINA_FALSE;

        set_autocap_mode();
    }
}

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

static String name_to_uuid(String name)
{
    String tmpUuid("");
    for(unsigned int i = 0;i<_names.size();i++)
    {
        if (strcmp(name.c_str(),_names[i].c_str())== 0) {
            tmpUuid = _uuids[i];
            break;
        }
    }
    return tmpUuid;
}

static void sw_keyboard_selection_view_back_cb (void *data, Evas_Object *obj, void *event_info)
{
    if (data == NULL)
        return;

    struct ug_data *ugd = (struct ug_data *)data;
#ifndef SINGLE_SELECTION
    String tmpStr = _config->read(SCIM_CONFIG_DEFAULT_HELPER_ISE, String("b70bf6cc-ff77-47dc-a137-60acc32d1e0c"));
    snprintf (_active_ise_name, sizeof (_active_ise_name), "%s", (uuid_to_name(tmpStr)).c_str());

    String ise_uuid;
    for (int i = 0; i< _names.size (); i++) {
        if (strcmp (_active_ise_name, _names[i].c_str ()) == 0) {
            ise_uuid = _uuids[i];
            break;
        }
    }

    if (std::find (enable_uuid_list.begin (), enable_uuid_list.end (), ise_uuid) == enable_uuid_list.end ()) {
        int next;
        String next_ise_uuid;
        String next_ise;
        //active next
        for (int i = 0; i < sw_iselist.size (); i++) {
            if (strcmp (_active_ise_name, sw_iselist[i].c_str ()) == 0) {
                next = i + 1;
                break;
            }
        }
        if (next >= sw_iselist.size ())
            next = 0;
        next_ise = sw_iselist[next];
        for (int i = 0; i < _names.size (); i++) {
            if (strcmp (next_ise.c_str (), _names[i].c_str ()) == 0) {
                next_ise_uuid = _uuids[i];
                break;
            }
        }

        while (std::find (enable_uuid_list.begin (), enable_uuid_list.end (), next_ise_uuid) == enable_uuid_list.end ()) {
            next ++;
            if (next >= sw_iselist.size ())
                next = 0;

            next_ise = sw_iselist[next];
            for (int i = 0; i < _names.size (); i++) {
                if (strcmp (next_ise.c_str (), _names[i].c_str ()) == 0) {
                    next_ise_uuid = _uuids[i];
                    break;
                }
            }
        }

        isf_control_set_active_ise_by_uuid (next_ise_uuid.c_str ());
    }

    _config->write (SCIM_CONFIG_ENABLED_ISE_UUID_LIST, enable_uuid_list);
    _config->flush ();

    int num = enable_uuid_list.size ();

    char tx[100] = {'\0'};
    snprintf (tx, sizeof (tx), "%d", num);
    edje_object_part_text_set (_EDJ (ugd->sw_ise_di), "on_off_text", tx);
#else
    snprintf (_active_ise_name, sizeof (_active_ise_name), "%s", ise_bak);
#endif

    ugd->key_end_cb = back_cb;
}

static void update_isf_setting_main_view(ug_data *ugd)
{
    p_items[SW_KEYBOARD_SEL_ITEM]->sub_text = strdup(_active_ise_name);
    elm_genlist_item_data_set(ugd->sw_ise_item_tizen,p_items[SW_KEYBOARD_SEL_ITEM]);
    p_items[HW_KEYBOARD_SEL_ITEM]->sub_text = strdup(_active_hw_ise_name);
    elm_genlist_item_data_set(ugd->hw_ise_item_tizen,p_items[HW_KEYBOARD_SEL_ITEM]);
}

static void sw_keyboard_selection_view_set_cb (void *data, Evas_Object *obj, void *event_info)
{
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
        isf_control_set_active_ise_by_uuid (uuid.c_str ());

        snprintf (ise_bak, sizeof (ise_bak), "%s", _active_ise_name);
    }

    update_isf_setting_main_view(ugd);

    ugd->key_end_cb = back_cb;
}

static void set_active_ise(int index)
{
    snprintf (_active_ise_name, sizeof (_active_ise_name), "%s", sw_iselist[index].c_str ());
}

static void _sw_radio_cb (void *data, Evas_Object *obj, void *event_info)
{
    int index = GPOINTER_TO_INT(data);
    elm_radio_value_set (sw_radio_grp, index);
    set_active_ise(index);
}

static void lang_view_back_cb  (void *data, Evas_Object *obj, void *event_info)
{
    common_ugd->key_end_cb = sw_keyboard_selection_view_set_cb ;
}

static void show_lang_cb (void *data, Evas_Object *obj, void *event_info)
{
    int index = GPOINTER_TO_INT(data);
    String langlist_str,normal_langlist_str;
    for (unsigned int i = 0; i < _names.size (); i++) {
       if (strcmp (_names[i].c_str (), sw_iselist[index].c_str()) == 0)
          langlist_str = _langs[i];
    }
    std::vector<String> langlist_vec,normal_langlist_vec;
    scim_split_string_list(langlist_vec,langlist_str);
    normal_langlist_str = ((scim_get_language_name(langlist_vec[0].c_str())).c_str()) ;
    for (unsigned int i = 1;i<langlist_vec.size();i++) {
        normal_langlist_str += String(", ");
        normal_langlist_str += ((scim_get_language_name(langlist_vec[i].c_str())).c_str());
    }

    Evas_Object *scroller = elm_scroller_add(common_ugd->naviframe);
    elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_FALSE);

    Evas_Object* layout = elm_layout_add(common_ugd->naviframe);
    elm_layout_file_set(layout, ISF_SETTING_EDJ, "isfsetting/languageview");

    Evas_Object *label = elm_label_add(layout);
    elm_label_line_wrap_set(label, ELM_WRAP_WORD);

    Evas_Coord win_w = 0, win_h = 0;
    ecore_x_window_size_get(ecore_x_window_root_first_get(), &win_w, &win_h);
    elm_label_wrap_width_set(label, win_w - PADDING_X * 2);
    elm_object_text_set(label, normal_langlist_str.c_str());
    elm_object_part_content_set(layout, "content", label);

    elm_object_content_set (scroller, layout);

    //Push the layout along with function buttons and title
    Elm_Object_Item *it = elm_naviframe_item_push(common_ugd->naviframe, _T(sw_iselist[index].c_str()), NULL, NULL, scroller, NULL);

    Evas_Object *back_btn = elm_object_item_content_part_get (it, ELM_NAVIFRAME_ITEM_PREV_BTN);
    evas_object_smart_callback_add (back_btn, "clicked", lang_view_back_cb, NULL);

    common_ugd->key_end_cb = lang_view_back_cb;
}

static char *_gl_label_get(void *data, Evas_Object *obj, const char *part)
{
    ItemData *item_data = (ItemData *)data;
    if (!strcmp(part, "elm.text")) {
        return strdup(item_data->text);
    }
    if (!strcmp(part,"elm.text.1")) {
        return strdup(item_data->text);
    }
    if (!strcmp(part , "elm.text.2")) {
        return strdup (item_data->sub_text);
    }
    return NULL;
}

static Evas_Object *_gl_icon_get(void *data, Evas_Object *obj, const char *part)
{
    ItemData *item_data = (ItemData *)data;

    if (!strcmp(part, "elm.icon")) {
        Evas_Object *onoff_ck = elm_check_add(obj);
        elm_object_style_set (onoff_ck, "on&off");
        if (item_data->mode == AUTO_CAPITALIZATION_ITEM) {
            elm_check_state_set(onoff_ck,f_auto_capitalization);
        }
        evas_object_smart_callback_add (onoff_ck, "changed", _onoff_cb, (void *)(item_data->mode));
        return onoff_ck;
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

static void _gl_sw_ise_sel(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Genlist_Item *item = (Elm_Genlist_Item *)event_info;
    elm_genlist_item_selected_set(item, EINA_FALSE);

    mark = (int) (data);
    set_active_ise(mark);

    elm_genlist_item_update(item);

    return;
}

static void _gl_hw_ise_sel(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Genlist_Item *item = (Elm_Genlist_Item *)event_info;
    elm_genlist_item_selected_set(item, EINA_FALSE);

    hw_mark = (int) (data);
    snprintf (_active_hw_ise_name, sizeof (_active_hw_ise_name), "%s", hw_iselist[hw_mark].c_str ());

    elm_genlist_item_update(item);

    return;
}

static void _gl_sel(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Genlist_Item *item = (Elm_Genlist_Item *)event_info;
    elm_genlist_item_selected_set(item, EINA_FALSE);

    int id = (int) (data);

    if (id ==  AUTO_CAPITALIZATION_ITEM) {
        f_auto_capitalization = (f_auto_capitalization == EINA_TRUE? EINA_FALSE:EINA_TRUE);
        set_autocap_mode();
    }

    elm_genlist_item_update(item);

    return;
}

static void _gl_keyboard_sel(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Genlist_Item *item = (Elm_Genlist_Item *)event_info;
    ug_data *ugd = (ug_data *)data;
    if (item == ugd->sw_ise_item_tizen)
        sw_keyboard_selection_view_tizen (ugd);
    if (item == ugd->hw_ise_item_tizen)
        hw_keyboard_selection_view_tizen(ugd);
    elm_genlist_item_selected_set(item, EINA_FALSE);
    return;
}

static void _gl_ise_option_sel(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Genlist_Item *item = (Elm_Genlist_Item *)event_info;
    ug_data *ugd = (ug_data *)data;
    const char *isename = NULL;
    if (item == ugd->sw_ise_opt_item_tizen)
        isename = _active_ise_name;
    else
        isename = _active_hw_ise_name;
    show_ise_option_module(ugd, isename);
    elm_genlist_item_selected_set(item, EINA_FALSE);
}

static char *_gl_exp_sw_label_get(void *data, Evas_Object *obj, const char *part)
{
    int index = (int)(data);
    if (!strcmp(part, "elm.text")) {
        return strdup(sw_iselist[index].c_str());
    }
    return NULL;
}

static Evas_Object *_gl_exp_sw_icon_get(void *data, Evas_Object *obj, const char *part)
{
    int index = (int)(data);
    if (!strcmp(part, "elm.icon.1")) {
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
    if (!strcmp(part,"elm.icon.2")) {
        Evas_Object *icon = elm_button_add(obj);
        elm_object_style_set(icon, "reveal");
        evas_object_smart_callback_add (icon, "clicked", show_lang_cb, (void *)(index));
        evas_object_propagate_events_set (icon, EINA_FALSE);//not propagate to genlist item.
        return icon;
    }
    return NULL;
}

static char *_gl_exp_hw_label_get(void *data, Evas_Object *obj, const char *part)
{
    int index = (int)(data);
    if (!strcmp(part, "elm.text")) {
        return strdup(hw_iselist[index].c_str());
    }
    return NULL;
}

static Evas_Object *_gl_exp_hw_icon_get(void *data, Evas_Object *obj, const char *part)
{
    if (!strcmp(part, "elm.icon")) {
        int index = (int)(data);
        Evas_Object *radio  = elm_radio_add (obj);
        elm_radio_state_value_set (radio, index);
        if (hw_radio_grp == NULL)
            hw_radio_grp = elm_radio_add(obj);
        elm_radio_group_add (radio, hw_radio_grp);
        evas_object_show (radio);
        evas_object_smart_callback_add (radio, "changed", _hw_radio_cb, (void *) (index));
        if (hw_mark == index) {
            elm_radio_value_set (hw_radio_grp, hw_mark);
        }
        return radio;
    }
    return NULL;
}

static void sw_keyboard_selection_view_tizen(ug_data * ugd)
{
    ugd->key_end_cb = sw_keyboard_selection_view_back_cb;

    if (sw_radio_grp != NULL)
    {
        evas_object_del(sw_radio_grp);
        sw_radio_grp = NULL;
    }

    Evas_Object *genlist = elm_genlist_add (ugd->naviframe);
    evas_object_show (genlist);

    //Push the layout along with function buttons and title
    Elm_Object_Item *it = elm_naviframe_item_push(ugd->naviframe, _T("Selection"), NULL, NULL, genlist, NULL);

    Evas_Object *back_btn = elm_object_item_content_part_get (it, ELM_NAVIFRAME_ITEM_PREV_BTN);
    evas_object_smart_callback_add (back_btn, "clicked", sw_keyboard_selection_view_set_cb, ugd);

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
    itc4.item_style = "dialogue/1text.2icon.2";
    itc4.func.label_get = _gl_exp_sw_label_get;
    itc4.func.icon_get = _gl_exp_sw_icon_get;
    itc4.func.state_get = _gl_state_get;
    itc4.func.del = _gl_del;
    for (i = 0;i < sw_iselist.size();i++) {
        elm_genlist_item_append(genlist, &itc4,
                (void *)(i), NULL, ELM_GENLIST_ITEM_NONE, _gl_sw_ise_sel,
                (void *)(i));
    }
}

static void response_cb (void *data, Evas_Object *obj, void *event_info)
{
    if ((int)event_info != 5)
        evas_object_del (obj);
}

//for naviframe l_btn
static void ise_option_view_set_cb (void *data, Evas_Object *obj, void *event_info)
{
    if (!data)
        return;

    struct ug_data *ugd = (struct ug_data *)data;
    ugd->key_end_cb = back_cb;
    mdl->save_config (_config);
    _config->reload ();

    String display_name = String (":13");
    const char *p = getenv ("DISPLAY");
    if (p != NULL)
        display_name = String (p);
    HelperAgent helper_agent;
    HelperInfo  helper_info ("ff110940-b8f0-4062-9ff6-a84f4f3setup", "ISF Setting", "", "", SCIM_HELPER_STAND_ALONE);
    int id = helper_agent.open_connection (helper_info, display_name);
    if (id == -1) {
        std::cerr << "    open_connection failed!!!!!!\n";
    } else {
        helper_agent.reload_config ();
        helper_agent.close_connection ();
    }
}

static void show_ise_option_module(ug_data *ugd , const char *isename)
{
    String mdl_name;
    for (unsigned int i = 0; i < _names.size (); i++) {
        if (strcmp (_names[i].c_str (), isename) == 0)
            mdl_name = _module_names[i];
    }

    char module_name[256];
    snprintf (module_name, sizeof (module_name), "%s-imengine-setup", mdl_name.c_str ());
    if (mdl)
    {
        delete mdl;
        mdl = NULL;
    }
    mdl = new SetupModule (String (module_name));
    if ( mdl == NULL || !mdl->valid ()) {
        Evas_Object *popup = elm_popup_add (ugd->naviframe);
        evas_object_size_hint_weight_set (popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        char tmp[256] = {'\0'};
        snprintf (tmp,sizeof(tmp),"%s: %s", isename ,_T("No option"));
        elm_popup_desc_set (popup, tmp);
        elm_popup_mode_set (popup, ELM_POPUP_TYPE_ALERT);
        elm_popup_timeout_set (popup, 3.0);
        evas_object_smart_callback_add (popup, "response", response_cb, NULL);
        evas_object_show (popup);
    } else {
        char title[256];
        snprintf (title, sizeof(title), _T("Options"));
        ugd->opt_eo = mdl->create_ui (ugd->layout_main, ugd->naviframe);
        mdl->load_config (_config);

        Elm_Object_Item *it = elm_naviframe_item_push (ugd->naviframe, title, NULL, NULL, ugd->opt_eo, NULL);

        Evas_Object *back_btn = elm_object_item_content_part_get (it, ELM_NAVIFRAME_ITEM_PREV_BTN);
        evas_object_smart_callback_add (back_btn, "clicked", ise_option_view_set_cb, ugd);
        ugd->key_end_cb = ise_option_view_set_cb;
    }
}

static void hw_keyboard_selection_view_back_cb (void *data, Evas_Object *obj, void *event_info)
{
    if (!data)
        return;

    struct ug_data *ugd = (struct ug_data *)data;

#ifdef SINGLE_SELECTION
    snprintf (_active_hw_ise_name, sizeof (_active_hw_ise_name), "%s", hw_ise_bak);
#endif
    ugd->key_end_cb = back_cb;
}

static void hw_keyboard_selection_view_set_cb (void *data, Evas_Object *obj, void *event_info)
{
    if (!data)
        return;

    struct ug_data *ugd = (struct ug_data *)data;
    if (strcmp (hw_ise_bak, _active_hw_ise_name) != 0) {
        //if _active_ise_name is changed , active _active_ise_name.
        //find the uuid of the active
        String uuid;
        for (unsigned int i = 0; i < _names.size (); i++) {
            if (strcmp (_names[i].c_str (), _active_hw_ise_name) == 0)
                uuid = _uuids[i];
        }
        isf_control_set_active_ise_by_uuid (uuid.c_str ());
        snprintf (hw_ise_bak, sizeof (hw_ise_bak), "%s", _active_hw_ise_name);
    }

    update_isf_setting_main_view(ugd);

    ugd->key_end_cb = back_cb;
}

static void _hw_radio_cb (void *data, Evas_Object *obj, void *event_info)
{
    int index = GPOINTER_TO_INT(data);
    elm_radio_value_set (hw_radio_grp, index);
    snprintf (_active_hw_ise_name, sizeof (_active_hw_ise_name), "%s", hw_iselist[index].c_str ());
}

static void hw_keyboard_selection_view_tizen(ug_data * ugd)
{
    ugd->key_end_cb = hw_keyboard_selection_view_back_cb;

    if (hw_radio_grp != NULL)
    {
        evas_object_del(hw_radio_grp);
        hw_radio_grp = NULL;
    }

    Evas_Object *genlist = elm_genlist_add(ugd->naviframe);
    evas_object_show(genlist);

    //Push the layout along with function buttons and title
    Elm_Object_Item *nf_it = elm_naviframe_item_push(ugd->naviframe, _T("Selection"), NULL, NULL, genlist, NULL);

    Evas_Object *back_btn = elm_object_item_content_part_get (nf_it, ELM_NAVIFRAME_ITEM_PREV_BTN);
    evas_object_smart_callback_add (back_btn, "clicked", hw_keyboard_selection_view_set_cb, ugd);

    unsigned int i = 0;

    hw_iselist.clear();
    std::vector<String> selected_langs, all_langs;

    isf_get_all_languages (all_langs);
    isf_get_keyboard_names_in_languages (all_langs, hw_iselist);

    if (hw_iselist.size () > 0) {
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
                    &itcSeparator,                 // item class
                    NULL,                   // data
                    NULL,
                    ELM_GENLIST_ITEM_NONE,
                    NULL,
                    NULL);
        elm_genlist_item_display_only_set(item, EINA_TRUE);
    }

    std::vector<String>::iterator it;
    it = std::find (hw_iselist.begin (), hw_iselist.end (), uuid_to_name(String("d75857a5-4148-4745-89e2-1da7ddaf7999")));
    hw_iselist.erase (it);

    std::sort (hw_iselist.begin (), hw_iselist.end ());

    for (i = 0; i < hw_iselist.size (); i++) {
        if (strcmp (_active_hw_ise_name, hw_iselist[i].c_str ()) == 0) {
            hw_mark = i;
            break;
        }
    }

    //set item class for 1text.1icon(text+radiobutton)
    itc5.item_style = "dialogue/1text.1icon.2";
    itc5.func.label_get = _gl_exp_hw_label_get;
    itc5.func.icon_get = _gl_exp_hw_icon_get;
    itc5.func.state_get = _gl_state_get;
    itc5.func.del = _gl_del;
    for (i = 0;i < hw_iselist.size();i++) {
        elm_genlist_item_append(genlist, &itc5,
                (void *)(i), NULL, ELM_GENLIST_ITEM_NONE, _gl_hw_ise_sel,
                (void *)(i));
    }
}

static Evas_Object *isf_setting_main_view_tizen (ug_data *ugd)
{
    Elm_Genlist_Item *item = NULL;

    if (ugd->naviframe == NULL)
    {
        ugd->naviframe = _create_naviframe_layout (ugd->layout_main);
        ugd->key_end_cb = back_cb;

        Evas_Object *genlist = elm_genlist_add(ugd->naviframe);
        //set item class for 1text.1icon(text+radiobutton)
        itc1.item_style = "dialogue/1text.1icon";
        itc1.func.label_get = _gl_label_get;
        itc1.func.icon_get = _gl_icon_get;
        itc1.func.state_get = _gl_state_get;
        itc1.func.del = _gl_del;

        //set item class for 2text(text+subtext)
        itc2.item_style = "dialogue/2text.3";
        itc2.func.label_get = _gl_label_get;
        itc2.func.icon_get = NULL;
        itc2.func.state_get = _gl_state_get;
        itc2.func.del = _gl_del;

        //set item class for normal text(text)
        itc3.item_style = "dialogue/1text";
        itc3.func.label_get = _gl_label_get;
        itc3.func.icon_get = NULL;
        itc3.func.state_get = _gl_state_get;
        itc3.func.del = _gl_del;

        // Set item class for dialogue group seperator
        itcSeparator.item_style = "dialogue/separator/21/with_line";
        itcSeparator.func.label_get = NULL;
        itcSeparator.func.icon_get = NULL;
        itcSeparator.func.state_get = NULL;
        itcSeparator.func.del = NULL;

        // Set item class for dialogue group title
        itcTitle.item_style = "dialogue/title";
        itcTitle.func.label_get = _gl_label_get;
        itcTitle.func.icon_get = NULL;
        itcTitle.func.state_get = _gl_state_get;
        itcTitle.func.del = _gl_del;
//==================================group begin =======================
        ItemData *item_data = NULL;

//separator
        item = elm_genlist_item_append(
                    genlist,                // genlist object
                    &itcSeparator,          // item class
                    NULL,                   // data
                    NULL,
                    ELM_GENLIST_ITEM_NONE,
                    NULL,
                    NULL);
        elm_genlist_item_display_only_set(item, EINA_TRUE);

//group1 item1
        item_data = (ItemData *)malloc(sizeof(ItemData));
        if (item_data != NULL) {
            memset(item_data,0,sizeof(ItemData));
            p_items[AUTO_CAPITALIZATION_ITEM] = item_data;
            item_data->text = strdup(_T("Auto capitalization"));
            item_data->mode = AUTO_CAPITALIZATION_ITEM;
            elm_genlist_item_append(
                    genlist,                // genlist object
                    &itc1,                  // item class
                    item_data,              // data
                    NULL,
                    ELM_GENLIST_ITEM_NONE,
                    _gl_sel,
                    (void *)(item_data->mode));
        }

//group2 title
        item_data = (ItemData *)malloc(sizeof(ItemData));
        if (item_data != NULL) {
            memset(item_data,0,sizeof(ItemData));
            p_items[SW_KEYBOARD_GROUP_TITLE_ITEM] = item_data;
            item_data->text = strdup(_T("Software keyboard"));
            item = elm_genlist_item_append(
                    genlist,                // genlist object
                    &itcTitle,              // item class
                    item_data,              // data
                    NULL,
                    ELM_GENLIST_ITEM_NONE,
                    NULL,
                    NULL);
            elm_genlist_item_display_only_set(item, EINA_TRUE);
        }

//group2 item1
        std::vector<String> ise_names;

        String tmpStr = _config->read(SCIM_CONFIG_DEFAULT_HELPER_ISE,String("b70bf6cc-ff77-47dc-a137-60acc32d1e0c"));
        snprintf (_active_ise_name, sizeof (_active_ise_name), "%s", (uuid_to_name(tmpStr)).c_str());
        snprintf (ise_bak, sizeof (ise_bak), "%s", _active_ise_name);

        item_data = (ItemData *)malloc(sizeof(ItemData));
        if (item_data!=NULL) {
            memset(item_data,0,sizeof(ItemData));
            p_items[SW_KEYBOARD_SEL_ITEM] = item_data;
            item_data->text = strdup(_T("Keyboard selection"));
            item_data->sub_text = strdup(_active_ise_name);
            item_data->mode = AUTO_CAPITALIZATION_ITEM;

            ugd->sw_ise_item_tizen = elm_genlist_item_append(
                    genlist,                // genlist object
                    &itc2,                  // item class
                    item_data,              // data
                    NULL,
                    ELM_GENLIST_ITEM_NONE,
                    _gl_keyboard_sel,
                    (void *)ugd);

            if (is_hw_connected)
                elm_genlist_item_disabled_set(ugd->sw_ise_item_tizen, EINA_TRUE);
        }

//group2 item2
        item_data = (ItemData *)malloc(sizeof(ItemData));
        if (item_data != NULL) {
            memset(item_data,0,sizeof(ItemData));
            p_items[SW_ISE_OPTION_ITEM] = item_data;
            item_data->text = strdup (_T("Keyboard option"));
            item_data->mode = SW_ISE_OPTION_ITEM;
            ugd->sw_ise_opt_item_tizen = elm_genlist_item_append(
                    genlist,                // genlist object
                    &itc3,                  // item class
                    item_data,              // data
                    NULL,
                    ELM_GENLIST_ITEM_NONE,
                    _gl_ise_option_sel,
                    (void *)ugd);

            if (is_hw_connected)
                elm_genlist_item_disabled_set(ugd->sw_ise_opt_item_tizen,EINA_TRUE);
        }

//group3 title
        item_data = (ItemData *)malloc(sizeof(ItemData));
        if (item_data != NULL) {
            memset(item_data,0,sizeof(ItemData));
            p_items[HW_KEYBOARD_GROUP_TITLE_ITEM] = item_data;
            item_data->text=strdup(_T("Hardware keyboard"));
            item = elm_genlist_item_append(
                    genlist,                // genlist object
                    &itcTitle,              // item class
                    item_data,              // data
                    NULL,
                    ELM_GENLIST_ITEM_NONE,
                    NULL,
                    NULL);
            elm_genlist_item_display_only_set(item, EINA_TRUE);
        }

//group3 item1
        String uuid,name;
        isf_get_keyboard_ise(uuid,name,_config);
        //temp
        if (strcmp(uuid.c_str(),"d75857a5-4148-4745-89e2-1da7ddaf7999") == 0)
        {
            name = name + String("English");
            uuid = name_to_uuid(name);
        }
       snprintf(_active_hw_ise_name,sizeof(_active_hw_ise_name),"%s",name.c_str());

        snprintf (hw_ise_bak, sizeof (hw_ise_bak), "%s", _active_hw_ise_name);

        item_data = (ItemData *)malloc(sizeof(ItemData));
        if (item_data != NULL) {
            memset(item_data,0,sizeof(ItemData));
            p_items[HW_KEYBOARD_SEL_ITEM] = item_data;
            item_data->text = strdup(_T("Keyboard selection"));
            item_data->sub_text = strdup(_active_hw_ise_name);
            ugd->hw_ise_item_tizen = elm_genlist_item_append(
                    genlist,                // genlist object
                    &itc2,                  // item class
                    item_data,              // data
                    NULL,
                    ELM_GENLIST_ITEM_NONE,
                    _gl_keyboard_sel,
                    (void *)ugd);

            if (!is_hw_connected)
                elm_genlist_item_disabled_set(ugd->hw_ise_item_tizen,EINA_TRUE);
        }
//group3 item2
            p_items[HW_ISE_OPTION_ITEM] = NULL;

//==================================group end =========================
        Evas_Object *back_btn = elm_button_add(ugd->naviframe);
        elm_object_style_set(back_btn, "naviframe/back_btn/default");
        evas_object_smart_callback_add (back_btn, "clicked", back_cb, ugd);

        elm_naviframe_item_push (ugd->naviframe, _T("Keyboard"), back_btn, NULL, genlist, NULL);
    }
    return ugd->naviframe;
}

static void sync_iselist()
{
    // Request ISF to update ISE list, below codes are very important, dont remove
    char **iselist = NULL;
    int count = isf_control_get_iselist (&iselist);
    int i;
    for (i = 0; i < count; i++) {
        SCIM_DEBUG_MAIN (3) << " [" << i << " : " << iselist[i] << "] \n";
        delete [] (iselist[i]);
    }
    if (iselist != NULL)
        free (iselist);
}

static void load_config_data (ConfigPointer config)
{
#ifndef SINGLE_SELECTION
    enable_uuid_list.clear ();

    enable_uuid_list = config->read (SCIM_CONFIG_ENABLED_ISE_UUID_LIST, enable_uuid_list);
    _default_ise_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID), String ("b70bf6cc-ff77-47dc-a137-60acc32d1e0c"));
    if (enable_uuid_list.size () == 0) {
        enable_uuid_list.push_back (_default_ise_uuid);

        isf_control_set_active_ise_by_uuid ( _default_ise_uuid.c_str ());
    }
#endif
    int tmp_cap=0;
    vconf_get_bool (VCONFKEY_SETAPPL_AUTOCAPITAL_ALLOW_BOOL, &tmp_cap);
    if (tmp_cap == true) f_auto_capitalization = EINA_TRUE;
}

ConfigPointer isf_imf_context_get_config (void);
static void load_config_module (void)
{
    _config = isf_imf_context_get_config ();
    if (_config.null ()) {
        std::cerr << "Create dummy config!!!\n";
        _config = new DummyConfig ();
    }

    if (_config.null ()) {
        std::cerr << "Can not create Config Object!\n";
    }
}

static void hw_connection_change_cb(ug_data *ugd)
{
    //enable / disable switch
    elm_genlist_item_disabled_set(ugd->sw_ise_item_tizen,!elm_genlist_item_disabled_get(ugd->sw_ise_item_tizen));
    elm_genlist_item_disabled_set(ugd->sw_ise_opt_item_tizen,!elm_genlist_item_disabled_get(ugd->sw_ise_opt_item_tizen));
    elm_genlist_item_disabled_set(ugd->hw_ise_item_tizen,!elm_genlist_item_disabled_get(ugd->hw_ise_item_tizen));
    elm_genlist_item_disabled_set(ugd->hw_ise_opt_item_tizen,!elm_genlist_item_disabled_get(ugd->hw_ise_opt_item_tizen));

    if (!is_hw_connected)
    {
        String uuid;
        for (unsigned int i = 0; i < _names.size (); i++) {
            if (strcmp (_names[i].c_str (), _active_ise_name) == 0)
                uuid = _uuids[i];
        }
        isf_control_set_active_ise_by_uuid (uuid.c_str());
    }
}

static Eina_Bool
_prop_change(void *data, int ev_type, void *ev)
{
    Ecore_X_Event_Window_Property *event = (Ecore_X_Event_Window_Property *)ev;
    unsigned int val = 0;

    if (event->win != _rootwin) return ECORE_CALLBACK_PASS_ON;
    if (event->atom != prop_x_ext_keyboard_exist) return ECORE_CALLBACK_PASS_ON;

    if (!ecore_x_window_prop_card32_get(event->win,
                                        prop_x_ext_keyboard_exist,
                                        &val, 1) > 0)
        return ECORE_CALLBACK_PASS_ON;

    if (val != 0) {
        is_hw_connected = true;
    }
    else
        is_hw_connected = false;

    ug_data *ugd = (ug_data *)data;
    hw_connection_change_cb(ugd);
    hw_kbd_num = val;

    return ECORE_CALLBACK_PASS_ON;
}

static void init_hw_keyboard_listener(ug_data *ugd)
{
    if (_prop_change_handler) return;

    _rootwin = ecore_x_window_root_first_get();
    ecore_x_event_mask_set(_rootwin, ECORE_X_EVENT_MASK_WINDOW_PROPERTY);

    _prop_change_handler = ecore_event_handler_add
                            (ECORE_X_EVENT_WINDOW_PROPERTY, _prop_change, (void *)ugd);

    if (!prop_x_ext_keyboard_exist)
        prop_x_ext_keyboard_exist = ecore_x_atom_get(PROP_X_EXT_KEYBOARD_EXIST);

    if (!ecore_x_window_prop_card32_get(_rootwin, prop_x_ext_keyboard_exist, &hw_kbd_num, 1)) {
        printf("Error! cannot get hw_kbd_num\n");
        return;
    }

    if (hw_kbd_num == 0)
        is_hw_connected = false;
    else
        is_hw_connected = true;
    return ;
}

static void *on_create (struct ui_gadget *ug, enum ug_mode mode, bundle *data, void *priv)
{
    Evas_Object *parent = NULL;
    Evas_Object *content = NULL;

    if (ug == NULL|| priv == NULL)
        return NULL;

    bindtextdomain (SETTING_PACKAGE, SETTING_LOCALEDIR);
    struct ug_data *ugd = (struct ug_data *)priv;
    ugd->ug = ug;
    parent = (Evas_Object *) ug_get_parent_layout (ug);
    if (parent == NULL)
        return NULL;

    if (imf_context == NULL) {
        const char *ctx_id = ecore_imf_context_default_id_get ();
        if (ctx_id) {
            imf_context = ecore_imf_context_add (ctx_id);
        }
    }

    load_config_module ();

    //only helper ISEs will be needed in isfsetting according to phone requirement.
    load_config_data (_config);
    sync_iselist();
    isf_load_ise_information (ALL_ISE, _config);
    init_hw_keyboard_listener(ugd);
    //-------------------------- ise infomation ----------------------------

    //construct the UI part of the isfsetting module
    if (mode == UG_MODE_FULLVIEW)
        ugd->layout_main = create_fullview (parent, ugd);
    else
        ugd->layout_main = create_frameview (parent, ugd);

    if (ugd->layout_main != NULL) {
        content = isf_setting_main_view_tizen(ugd);
        elm_object_part_content_set (ugd->layout_main, "elm.swallow.content", content);
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
    if (ug == NULL|| priv == NULL)
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

    if (ugd->opt_eo != NULL) {
        evas_object_del(ugd->opt_eo);
        ugd->opt_eo = NULL;
    }

    if (mdl) {
        delete mdl;
        mdl = NULL;
    }

    if (!_config.null ()) {
        _config->flush ();
        _config.reset ();
    }

    for(int i = 0 ;i < ITEM_TOTAL_COUNT;i++)
    {
        if (p_items[i]!= NULL)
        {
            if (p_items[i]->text) {
                free(p_items[i]->text);
                p_items[i]->text = NULL;
            }
            if (p_items[i]->sub_text) {
                free(p_items[i]->sub_text);
                p_items[i]->sub_text = NULL;
            }
            free(p_items[i]);
            p_items[i]= NULL;
        }
    }

    if (!_prop_change_handler) return;
    ecore_event_handler_del(_prop_change_handler);
    _prop_change_handler = NULL;
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
    if ( ug == NULL || priv == NULL)
        return ;

    struct ug_data *ugd = (struct ug_data *)priv;
    Elm_Object_Item *top_it;

    switch (event) {
        case UG_KEY_EVENT_END:
            top_it = elm_naviframe_top_item_get(ugd->naviframe);
            if (top_it && (elm_object_item_content_get(top_it) != ugd->opt_eo) && (ugd->key_end_cb == ise_option_view_set_cb))//ise option maybe multiple layouts
            {
                mdl->key_proceeding(TYPE_KEY_END);
            }
            else
            {
                ugd->key_end_cb(priv, NULL, NULL);
            }

            break;
        default:
            break;
    }
}

#ifdef __cplusplus
extern "C"
{
#endif
    UG_MODULE_API int UG_MODULE_INIT (struct ug_module_ops *ops) {
        if (ops == NULL)
            return -1;

        struct ug_data *ugd = (ug_data*)calloc (1, sizeof (struct ug_data));
        if (ugd == NULL)
            return -1;

        common_ugd = ugd;
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

    UG_MODULE_API void UG_MODULE_EXIT (struct ug_module_ops *ops) {
        if (ops == NULL)
            return;

        struct ug_data *ugd = (struct ug_data *)(ops->priv);
        if (ugd != NULL)
            free (ugd);
    }

    UG_MODULE_API int setting_plugin_reset(bundle *data, void *priv)//for setting keyboard reset
    {
        if (vconf_set_bool (VCONFKEY_SETAPPL_AUTOCAPITAL_ALLOW_BOOL, false) == -1)
            return -1;

        load_config_module ();
        load_config_data (_config);
        isf_load_ise_information (ALL_ISE, _config);

        String uuid =  scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID), String ("b70bf6cc-ff77-47dc-a137-60acc32d1e0c"));
        isf_control_set_active_ise_by_uuid (uuid.c_str());

        String mdl_name;

        for (unsigned int i = 0; i < _module_names.size (); i++) {
            mdl_name = _module_names[i];
            char module_name[256];
            snprintf (module_name, sizeof (module_name), "%s-imengine-setup", mdl_name.c_str ());
            mdl = new SetupModule (String (module_name));
            if ( mdl == NULL || !mdl->valid ()) {
                if (mdl)
                {
                    delete mdl;
                    mdl = NULL;
                }
                continue;
            }
            else{
                if (mdl->option_reset(_config) == false)
                {
                    printf("mdl %s failed to reset option!\n",module_name);
                    if (mdl)
                    {
                        delete mdl;
                        mdl = NULL;
                    }
                    return -1;
                }
                else
                {
                    _config->reload ();
                    if (mdl)
                    {
                        delete mdl;
                        mdl = NULL;
                    }
                }
            }
        }
        return 0;
    }

#ifdef __cplusplus
}
#endif

/*
vi:ts=4:ai:nowrap:expandtab
*/
