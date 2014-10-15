/*
 * Copyright (c) 2012 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <string.h>
#include <scl.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <Ecore_X.h>

#include <glib.h>
#include <Elementary.h>
#include <efl_assist.h>
#include <vconf.h>

#include "option.h"
#include "languages.h"

using namespace scl;

static ISELanguageManager _language_manager;

#define OPTION_MAX_LANGUAGES 255

enum SETTING_ITEM_ID {
    SETTING_ITEM_ID_LANGUAGE,

    SETTING_ITEM_ID_MAX,
};

enum LANGUAGE_VIEW_MODE {
    LANGUAGE_VIEW_MODE_LANGUAGE,
    LANGUAGE_VIEW_MODE_INPUT_MODE,
};

static ITEMDATA main_itemdata[SETTING_ITEM_ID_MAX];
static ITEMDATA language_itemdata[OPTION_MAX_LANGUAGES];

struct OPTION_ELEMENTS
{
    OPTION_ELEMENTS() {
        option_window = NULL;
        naviframe = NULL;
        lang_popup = NULL;

        event_handler = NULL;

        itc_main_separator = NULL;
        itc_main_text_only = NULL;

        itc_language_subitems = NULL;
        itc_language_radio = NULL;

        languages_item = NULL;

        memset(language_item, 0x00, sizeof(language_item));
        memset(rdg_language, 0x00, sizeof(rdg_language));
    }
    Evas_Object *option_window;
    Evas_Object *naviframe;
    Evas_Object *lang_popup;

    Ecore_Event_Handler *event_handler;

    Elm_Genlist_Item_Class *itc_main_separator;
    Elm_Genlist_Item_Class *itc_main_text_only;

    Elm_Genlist_Item_Class *itc_language_subitems;
    Elm_Genlist_Item_Class *itc_language_radio;

    Elm_Object_Item *languages_item;

    Elm_Object_Item *language_item[OPTION_MAX_LANGUAGES];
    Evas_Object *rdg_language[OPTION_MAX_LANGUAGES];
};

static OPTION_ELEMENTS ad;
extern CONFIG_VALUES g_config_values;

//static Evas_Object* create_main_window();
static Evas_Object* create_option_language_view(Evas_Object *naviframe);

/* This function is called by setup_module.cpp : create_ise_setup_eo() also */
Evas_Object* create_option_main_view(Evas_Object *parent, Evas_Object *naviframe);

static void input_mode_selected(void *data, Evas_Object *obj, void *event_info);
static void language_selected(void *data, Evas_Object *obj, void *event_info);

static void navi_back_cb(void *data, Evas_Object *obj, void *event_info);

std::vector<ILanguageOption*> LanguageOptionManager::language_option_vector;
void LanguageOptionManager::add_language_option(ILanguageOption *language_option) {
    language_option_vector.push_back(language_option);
}
scluint LanguageOptionManager::get_language_options_num() {
    return language_option_vector.size();
}
ILanguageOption* LanguageOptionManager::get_language_option_info(scluint index) {
    return ((index < language_option_vector.size()) ? language_option_vector.at(index) : NULL);
}

static Evas_Object*
create_main_window(int degree)
{
    Evas_Object *window = elm_win_util_standard_add("Option window", "Option window");

    const char *szProfile[] = {"mobile", ""};
    elm_win_profiles_set(window, szProfile, 1);

    elm_win_borderless_set(window, EINA_TRUE);

    Evas_Coord win_w = 0, win_h = 0;
    ecore_x_window_size_get(ecore_x_window_root_first_get(), &win_w, &win_h);
    if(degree == 90 || degree == 270){
        evas_object_resize(window, win_h, win_w);
    }else{
        evas_object_resize(window, win_w, win_h);
    }

    int rots[] = { 0, 90, 180, 270 };
    elm_win_wm_rotation_available_rotations_set(window, rots, (sizeof(rots) / sizeof(int)));

    evas_object_show(window);

    return window;
}

static char *_main_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
    ITEMDATA *item_data = (ITEMDATA*)data;
    if (item_data) {
        if (!strcmp(part, "elm.text")) {
            return strdup(item_data->main_text);
        }
        if (!strcmp(part, "elm.text.1")) {
            return strdup(item_data->main_text);
        }
        if (!strcmp(part, "elm.text.2")) {
            return strdup(item_data->sub_text);
        }
    }
    return NULL;
}

static Evas_Object *_main_gl_content_get(void *data, Evas_Object *obj, const char *part)
{
    Evas_Object *item = NULL;
    ITEMDATA *item_data = (ITEMDATA*)data;

    if (item_data) {
        if (!strcmp(part, "elm.icon")) {
        }
    }

    return item;
}

static void _main_gl_sel(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *item = (Elm_Object_Item*)event_info;

    int id = (int)(data);

    switch (id) {
        case SETTING_ITEM_ID_LANGUAGE: {
            create_option_language_view(ad.naviframe);
        }
        break;
        default:
        break;
    }

    if (item) {
        elm_genlist_item_selected_set(item, EINA_FALSE);
    }
}

static Eina_Bool _main_gl_state_get(void *data, Evas_Object *obj, const char *part)
{
    return EINA_FALSE;
}

static void _main_gl_del(void *data, Evas_Object *obj)
{
    return;
}

static void _main_gl_con(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *item = (Elm_Object_Item*)event_info;
    elm_genlist_item_subitems_clear(item);
}

static void _main_gl_exp(void *data, Evas_Object *obj, void *event_info)
{
    // Elm_Object_Item *it = (Elm_Object_Item*)event_info;
    // Evas_Object *gl =  elm_object_item_widget_get(it);
}

static char *_input_mode_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
    if (data) {
        for (unsigned int loop = 0;loop < OPTION_MAX_LANGUAGES && loop < _language_manager.get_languages_num();loop++) {
            LANGUAGE_INFO *info = _language_manager.get_language_info(loop);
            if (info) {
                for (unsigned int inner_loop = 0;inner_loop < info->input_modes.size();inner_loop++) {
                    if (info->input_modes.at(inner_loop).name.compare((const char*)data) == 0) {
                        return strdup(info->input_modes.at(inner_loop).display_name.c_str());
                    }
                }
            }
        }
    }
    return NULL;
}

static Evas_Object *_input_mode_gl_content_get(void *data, Evas_Object *obj, const char *part)
{
    Evas_Object *item = NULL;

    if (data) {
        if (strcmp(part, "elm.icon") == 0) {
            for (unsigned int loop = 0;loop < OPTION_MAX_LANGUAGES && loop < _language_manager.get_languages_num();loop++) {
                LANGUAGE_INFO *info = _language_manager.get_language_info(loop);
                if (info) {
                    for (unsigned int inner_loop = 0;inner_loop < info->input_modes.size();inner_loop++) {
                        if (info->input_modes.at(inner_loop).name.compare((const char*)data) == 0) {
                            item = elm_radio_add(obj);
                            elm_radio_state_value_set(item, inner_loop);
                            if (inner_loop == 0) {
                                ad.rdg_language[loop] = item;
                            }
                            elm_radio_group_add(item, ad.rdg_language[loop]);
                            evas_object_smart_callback_add(item, "changed", input_mode_selected, data);
                        }
                    }
                }
            }
        }
    }

    return item;
}

/*static void _input_mode_gl_sel(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *item = (Elm_Object_Item*)event_info;

    if (data) {
        for (unsigned int loop = 0;loop < OPTION_MAX_LANGUAGES && loop < _language_manager.get_languages_num();loop++) {
            LANGUAGE_INFO *info = _language_manager.get_language_info(loop);
            if (info) {
                for (unsigned int inner_loop = 0;inner_loop < info->input_modes.size();inner_loop++) {
                    if (info->input_modes.at(inner_loop).name.compare((const char*)data) == 0) {
                        input_mode_selected(data, NULL, NULL);
                        elm_radio_value_set(ad.rdg_language[loop], inner_loop);
                        elm_genlist_item_update(ad.language_item[loop]);
                    }
                }
            }
        }
    }
    if (item) {
        elm_genlist_item_selected_set(item, EINA_FALSE);
    }
}*/

static Eina_Bool _input_mode_gl_state_get(void *data, Evas_Object *obj, const char *part)
{
    return EINA_FALSE;
}

static void _input_mode_gl_del(void *data, Evas_Object *obj)
{
    return;
}

static char *_language_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
    ITEMDATA *item_data = (ITEMDATA*)data;
    if (item_data) {
        if (!strcmp(part, "elm.text")) {
            return strdup(item_data->main_text);
        }
        if (!strcmp(part, "elm.text.1")) {
            return strdup(item_data->main_text);
        }
        if (!strcmp(part, "elm.text.2")) {
            return strdup(item_data->sub_text);
        }
    }
    return NULL;
}

static Evas_Object *_language_gl_content_get(void *data, Evas_Object *obj, const char *part)
{
    Evas_Object *item = NULL;
    ITEMDATA *item_data = (ITEMDATA*)data;

    if (item_data) {
        if (strcmp(part, "elm.icon") == 0) {
            if (item_data->mode >= 0 && item_data->mode < OPTION_MAX_LANGUAGES) {
                LANGUAGE_INFO *info = _language_manager.get_language_info(item_data->mode);
                if (info) {
                    Evas_Object *ck = elm_check_add(obj);
                    elm_object_style_set(ck, "default/genlist");
                    //elm_check_state_pointer_set(ck, &(g_language_state[item_data->mode]));
                    evas_object_repeat_events_set(ck, EINA_TRUE);
                    evas_object_propagate_events_set(ck, EINA_FALSE);
                    if (info->enabled) {
                        elm_check_state_set(ck, TRUE);
                    } else {
                        elm_check_state_set(ck, FALSE);
                    }
                    evas_object_smart_callback_add(ck, "changed", language_selected, data);
                    evas_object_show(ck);
                    item = ck;
                }
            }
        }
    }

    return item;
}

static void _language_gl_sel(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *item = (Elm_Object_Item*)event_info;
    ITEMDATA * item_data = (ITEMDATA*)elm_object_item_data_get(item);
    if (item) {
        elm_genlist_item_selected_set(item, EINA_FALSE);

        // Update check button
        Evas_Object *ck = elm_object_item_part_content_get(item, "elm.icon");
        Eina_Bool state = elm_check_state_get(ck);
        elm_check_state_set(ck, !state);
        language_selected(item_data, ck, NULL);
    }
}

#if 0
static Eina_Bool _language_gl_state_get(void *data, Evas_Object *obj, const char *part)
{
    return EINA_FALSE;
}

static void _language_gl_del(void *data, Evas_Object *obj)
{
    return;
}

static void _language_gl_con(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *item = (Elm_Object_Item*)event_info;
    elm_genlist_item_subitems_clear(item);
}

static void _language_gl_exp(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *it = (Elm_Object_Item*)event_info;
    Evas_Object *gl =  elm_object_item_widget_get(it);

    for (unsigned int loop = 0;loop < OPTION_MAX_LANGUAGES && loop < _language_manager.get_languages_num();loop++) {
        LANGUAGE_INFO *info = _language_manager.get_language_info(loop);
        if (info && it == ad.language_item[loop]) {
            int selected = 0;
            for (unsigned int inner_loop = 0;inner_loop < info->input_modes.size();inner_loop++) {
                if (info->selected_input_mode.compare(info->input_modes.at(inner_loop).name) == 0) {
                    selected = inner_loop;
                }
                elm_genlist_item_append(gl, ad.itc_language_radio, info->input_modes.at(inner_loop).name.c_str(),
                    it, ELM_GENLIST_ITEM_NONE, _input_mode_gl_sel, info->input_modes.at(inner_loop).name.c_str());
            }
            elm_radio_value_set(ad.rdg_language[loop], selected);
        }
    }
}
#endif
static void
destroy_genlist_item_classes() {
    if (ad.itc_main_separator) {
        elm_genlist_item_class_free(ad.itc_main_separator);
        ad.itc_main_separator = NULL;
    }
    if (ad.itc_main_text_only) {
        elm_genlist_item_class_free(ad.itc_main_text_only);
        ad.itc_main_text_only = NULL;
    }
    if (ad.itc_language_subitems) {
        elm_genlist_item_class_free(ad.itc_language_subitems);
        ad.itc_language_subitems = NULL;
    }
    if (ad.itc_language_radio) {
        elm_genlist_item_class_free(ad.itc_language_radio);
        ad.itc_language_radio = NULL;
    }
}

static void
create_genlist_item_classes() {
    ad.itc_main_separator = elm_genlist_item_class_new();
    if (ad.itc_main_separator) {
        ad.itc_main_separator->item_style = "dialogue/separator";
        ad.itc_main_separator->func.text_get = NULL;
        ad.itc_main_separator->func.content_get = NULL;
        ad.itc_main_separator->func.state_get = NULL;
        ad.itc_main_separator->func.del = NULL;
    }

    ad.itc_main_text_only = elm_genlist_item_class_new();
    if (ad.itc_main_text_only) {
        ad.itc_main_text_only->item_style = "dialogue/2text.3";
        ad.itc_main_text_only->func.text_get = _main_gl_text_get;
        ad.itc_main_text_only->func.content_get = _main_gl_content_get;
        ad.itc_main_text_only->func.state_get = _main_gl_state_get;
        ad.itc_main_text_only->func.del = _main_gl_del;
    }

    ad.itc_language_subitems = elm_genlist_item_class_new();
    if (ad.itc_language_subitems) {
        ad.itc_language_subitems->item_style = "dialogue/1text.1icon.3";
        ad.itc_language_subitems->func.text_get = _language_gl_text_get;
        ad.itc_language_subitems->func.content_get = _language_gl_content_get;
    }

    ad.itc_language_radio = elm_genlist_item_class_new();
    if (ad.itc_language_radio) {
        ad.itc_language_radio->item_style = "dialogue/1text.1icon/expandable2";
        ad.itc_language_radio->func.text_get = _input_mode_gl_text_get;
        ad.itc_language_radio->func.content_get = _input_mode_gl_content_get;
        ad.itc_language_radio->func.state_get = _input_mode_gl_state_get;
        ad.itc_language_radio->func.del = _input_mode_gl_del;
    }
}

static std::string compose_selected_languages_string()
{
    const int NUM_DISPLAY_LANGUAGE = 2;

    const int TEMP_STRING_LEN = 255;
    char szTemp[TEMP_STRING_LEN];

    std::string languages;
    int num_languages = 0;

    for (scluint loop = 0;loop < _language_manager.get_languages_num();loop++) {
        LANGUAGE_INFO *info = _language_manager.get_language_info(loop);
        if (info) {
            if (info->enabled) {
                num_languages++;
                if (num_languages <= NUM_DISPLAY_LANGUAGE) {
                    if (num_languages > 1) {
                        languages += ", ";
                    }
                    languages += info->display_name;
                }
            }
        }
    }
    if (num_languages > NUM_DISPLAY_LANGUAGE) {
        snprintf(szTemp, TEMP_STRING_LEN, "%d (%s, ...)", num_languages, languages.c_str());
    } else {
        snprintf(szTemp, TEMP_STRING_LEN, "%d (%s)", num_languages, languages.c_str());
    }

    return std::string(szTemp);
}

static void
language_selection_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
    if (_language_manager.get_enabled_languages_num() == 0) {
        read_ise_config_values();
        _language_manager.set_enabled_languages(g_config_values.enabled_languages);
    }
    std::string languages = compose_selected_languages_string();
    strncpy(main_itemdata[SETTING_ITEM_ID_LANGUAGE].sub_text, languages.c_str(), ITEM_DATA_STRING_LEN - 1);

    if (obj) {
        evas_object_smart_callback_del(obj, "clicked", language_selection_finished_cb);
    }

    sclboolean selected_language_found = FALSE;
    std::vector<std::string> enabled_languages;

    for (unsigned int loop = 0;loop < OPTION_MAX_LANGUAGES && loop < _language_manager.get_languages_num();loop++) {
        LANGUAGE_INFO *info = _language_manager.get_language_info(loop);
        if (info) {
            if (info->enabled) {
                enabled_languages.push_back(info->name);
                if (info->name.compare(g_config_values.selected_language) == 0) {
                    selected_language_found = TRUE;
                }
            }
        }
    }
    if (enabled_languages.size() > 0) {
        g_config_values.enabled_languages = enabled_languages;
        if (!selected_language_found) {
            if (!(g_config_values.enabled_languages.at(0).empty())) {
                g_config_values.selected_language = g_config_values.enabled_languages.at(0);
            }
        }

        write_ise_config_values();
    }
}

static void _popup_timeout_cb(void *data, Evas_Object * obj, void *event_info)
{
    if (obj) {
        evas_object_smart_callback_del(obj, "timeout", _popup_timeout_cb);
        evas_object_del(obj);
        ad.lang_popup = NULL;
    }
}


Eina_Bool _pop_cb(void *data, Elm_Object_Item *it)
{
    language_selection_finished_cb(NULL, NULL, NULL);
    if (ad.lang_popup) {
        _popup_timeout_cb(NULL, ad.lang_popup, NULL);
    }
    return EINA_TRUE;
}

static void
_naviframe_back_cb (void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *top_it = elm_naviframe_top_item_get(obj);
    Elm_Object_Item *bottom_it = elm_naviframe_bottom_item_get(obj);
    if (top_it && bottom_it &&
            (elm_object_item_content_get(top_it) == elm_object_item_content_get(bottom_it))) {
        close_option_window();
    } else {
        elm_naviframe_item_pop(obj);
        read_options();
    }
}

Evas_Object* create_option_main_view(Evas_Object *parent, Evas_Object *naviframe)
{
    create_genlist_item_classes();
    ad.naviframe = naviframe;

    Evas_Object *genlist = elm_genlist_add(naviframe);
    evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_object_style_set(genlist, "dialogue");
    elm_genlist_tree_effect_enabled_set(genlist, EINA_FALSE);

    Elm_Object_Item *separator = elm_genlist_item_append(genlist, ad.itc_main_separator, NULL, NULL,
        ELM_GENLIST_ITEM_NONE, NULL, NULL);
    elm_genlist_item_select_mode_set(separator, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

    if (_language_manager.get_languages_num() > 1) {
        std::string languages = compose_selected_languages_string();

        strncpy(main_itemdata[SETTING_ITEM_ID_LANGUAGE].main_text, LANGUAGE, ITEM_DATA_STRING_LEN - 1);
        strncpy(main_itemdata[SETTING_ITEM_ID_LANGUAGE].sub_text, languages.c_str(), ITEM_DATA_STRING_LEN - 1);
        main_itemdata[SETTING_ITEM_ID_LANGUAGE].mode = SETTING_ITEM_ID_LANGUAGE;
        ad.languages_item = elm_genlist_item_append(genlist, ad.itc_main_text_only, &main_itemdata[SETTING_ITEM_ID_LANGUAGE],
            NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, (void*)(main_itemdata[SETTING_ITEM_ID_LANGUAGE].mode));
    }

    evas_object_smart_callback_add(genlist, "expanded", _main_gl_exp, genlist);
    evas_object_smart_callback_add(genlist, "contracted", _main_gl_con, genlist);

    return genlist;
}

static Evas_Object* create_option_language_view(Evas_Object *naviframe)
{
    Evas_Object *genlist = elm_genlist_add(naviframe);
    elm_object_style_set(genlist, "dialogue");

    Elm_Object_Item *separator = elm_genlist_item_append(genlist, ad.itc_main_separator, NULL, NULL,
        ELM_GENLIST_ITEM_NONE, NULL, NULL);
    elm_genlist_item_select_mode_set(separator, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

    for (unsigned int loop = 0; loop < OPTION_MAX_LANGUAGES && loop < _language_manager.get_languages_num(); loop++)
    {
        LANGUAGE_INFO *info = _language_manager.get_language_info(loop);
        if (info) {
            strncpy(language_itemdata[loop].main_text, info->display_name.c_str(), ITEM_DATA_STRING_LEN - 1);
            language_itemdata[loop].mode = loop;
            ad.language_item[loop] = elm_genlist_item_append(genlist, ad.itc_language_subitems, &language_itemdata[loop], NULL,
                ELM_GENLIST_ITEM_NONE, _language_gl_sel, (void*)(language_itemdata[loop].mode));
        } else {
            ad.language_item[loop] = NULL;
        }
    }

    evas_object_show(genlist);

    Elm_Object_Item *navi_it = elm_naviframe_item_push(ad.naviframe, LANGUAGE, NULL, NULL, genlist,NULL);
    Evas_Object *back_btn = elm_object_item_part_content_get(navi_it, "elm.swallow.prev_btn");
    evas_object_data_set(genlist, "back_button", back_btn);
    evas_object_smart_callback_add (back_btn, "clicked", language_selection_finished_cb, NULL);
    elm_naviframe_item_pop_cb_set(navi_it, _pop_cb, NULL);

    return genlist;
}

void
close_option_window()
{
    if (ad.option_window) {
        evas_object_del(ad.option_window);
        ad.option_window = NULL;
    }

    if (ad.event_handler) {
        ecore_event_handler_del(ad.event_handler);
        ad.event_handler = NULL;
    }

    destroy_genlist_item_classes();
}

void read_options()
{
    std::string languages = compose_selected_languages_string();
    strncpy(main_itemdata[SETTING_ITEM_ID_LANGUAGE].sub_text, languages.c_str(), ITEM_DATA_STRING_LEN - 1);

    Elm_Object_Item *top_it;
    top_it = elm_naviframe_top_item_get (ad.naviframe);
    Evas_Object *content;
    content = elm_object_item_content_get (top_it);
    if (content) {
        Elm_Object_Item *item = elm_genlist_first_item_get (content);
        while (item) {
            elm_genlist_item_update (item);
            item = elm_genlist_item_next_get (item);
        }
    }
}

void write_options()
{
    language_selection_finished_cb(NULL, NULL, NULL);
}

static void set_option_values()
{
    std::string languages = compose_selected_languages_string();
    strncpy(main_itemdata[SETTING_ITEM_ID_LANGUAGE].sub_text, languages.c_str(), ITEM_DATA_STRING_LEN - 1);
}

static void input_mode_selected(void *data, Evas_Object *obj, void *event_info)
{
    if (data) {
        for (unsigned int loop = 0;loop < OPTION_MAX_LANGUAGES && loop < _language_manager.get_languages_num();loop++) {
            LANGUAGE_INFO *info = _language_manager.get_language_info(loop);
            if (info) {
                for (unsigned int inner_loop = 0;inner_loop < info->input_modes.size();inner_loop++) {
                    if (info->input_modes.at(inner_loop).name.compare((const char*)data) == 0) {
                        info->selected_input_mode = (const char*)data;

                        strncpy(language_itemdata[loop].sub_text,
                            info->input_modes.at(inner_loop).display_name.c_str(), ITEM_DATA_STRING_LEN - 1);
                    }
                }
            }
        }
    }

    std::string languages = compose_selected_languages_string();
    strncpy(main_itemdata[SETTING_ITEM_ID_LANGUAGE].sub_text, languages.c_str(), ITEM_DATA_STRING_LEN - 1);
}

static void language_selected(void *data, Evas_Object *obj, void *event_info)
{
    ITEMDATA *item_data = (ITEMDATA*)data;
    if (item_data) {
        LANGUAGE_INFO *info = _language_manager.get_language_info(item_data->mode);
        if (info) {
            info->enabled = elm_check_state_get(obj);
            if (!elm_check_state_get(obj)) {
                if (_language_manager.get_enabled_languages_num() == 0) {
                    info->enabled = TRUE;
                    elm_check_state_set(obj, EINA_TRUE);

                    ad.lang_popup = elm_popup_add(ad.naviframe);
                    elm_object_text_set(ad.lang_popup, MSG_NONE_SELECTED);
                    elm_popup_timeout_set(ad.lang_popup, 3.0);
                    evas_object_smart_callback_add(ad.lang_popup, "timeout", _popup_timeout_cb, ad.naviframe);
                    elm_object_focus_allow_set(ad.lang_popup, EINA_TRUE);
                    evas_object_show(ad.lang_popup);
                    ea_object_event_callback_add(ad.lang_popup, EA_CALLBACK_BACK, _popup_timeout_cb, NULL);
                }
            }
        }
    }
}

static Eina_Bool focus_out_cb(void *data, int type, void *event)
{
    language_selection_finished_cb(NULL, NULL, NULL);
    close_option_window();
    return ECORE_CALLBACK_CANCEL;
}

static void
navi_back_cb(void *data, Evas_Object *obj, void *event_info)
{
    evas_object_smart_callback_del(obj, "clicked", navi_back_cb);
    close_option_window();
}

#ifndef APPLY_WINDOW_MANAGER_CHANGE
static void
set_transient_for_app_window(Evas_Object *window)
{
    /* Set a transient window for window stack */
    /* Gets the current XID of the active window into the root window property  */
    Atom type_return;
    unsigned long nitems_return;
    unsigned long bytes_after_return;
    int format_return;
    unsigned char *data = NULL;
    Ecore_X_Window xAppWindow;
    Ecore_X_Window xWindow = elm_win_xwindow_get(window);
    gint ret = 0;

    ret = XGetWindowProperty ((Display *)ecore_x_display_get(), ecore_x_window_root_get(xWindow),
        ecore_x_atom_get("_ISF_ACTIVE_WINDOW"),
        0, G_MAXLONG, False, XA_WINDOW, &type_return,
        &format_return, &nitems_return, &bytes_after_return,
        &data);

    if (ret == Success) {
        if (data) {
            if (type_return == XA_WINDOW) {
                xAppWindow = *(Window *)data;
                LOGD("TRANSIENT_FOR SET : %x , %x", xAppWindow, xWindow);
                ecore_x_icccm_transient_for_set(xWindow, xAppWindow);
            }
            XFree(data);
        }
    }
}
#endif

void
open_option_window(Evas_Object *parent, sclint degree)
{
    read_ise_config_values();

    /* To make sure there is no temporary language in the enabled language list */
    _language_manager.set_enabled_languages(g_config_values.enabled_languages);

    set_option_values();

    if (ad.option_window)
    {
        elm_win_raise(ad.option_window);
    }
    else
    {
        memset(&ad, 0x0, sizeof(OPTION_ELEMENTS));

        /* create option window */
        Evas_Object *window = create_main_window(degree);
        ad.option_window = window;

        elm_win_indicator_mode_set (window, ELM_WIN_INDICATOR_SHOW);

        Evas_Object *layout = elm_layout_add(window);
        elm_layout_theme_set (layout, "layout", "application", "default");
        evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_show(layout);

        /* put the layout inside conformant for drawing indicator in app side */
        Evas_Object *conformant = elm_conformant_add(window);
        evas_object_size_hint_weight_set(conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(conformant, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_win_resize_object_add(window, conformant);
        elm_win_conformant_set(window, EINA_TRUE);
        evas_object_show(conformant);

        elm_object_content_set(conformant, layout);

        Evas_Object *naviframe = elm_naviframe_add(layout);
        elm_naviframe_prev_btn_auto_pushed_set(naviframe, EINA_FALSE);
        ea_object_event_callback_add(naviframe, EA_CALLBACK_BACK, _naviframe_back_cb, NULL);
        evas_object_size_hint_weight_set(naviframe, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(naviframe, EVAS_HINT_FILL, EVAS_HINT_FILL);

        Evas_Object *list = create_option_main_view(layout, naviframe);

        /* add a back button to naviframe */
        Evas_Object *back_btn = elm_button_add(naviframe);
        elm_object_style_set(back_btn, "naviframe/back_btn/default");
        evas_object_smart_callback_add (back_btn, "clicked", navi_back_cb, NULL);
        elm_naviframe_item_push(naviframe, OPTIONS, back_btn, NULL, list, NULL);

        elm_object_part_content_set(layout, "elm.swallow.content", naviframe);

        evas_object_show(naviframe);

        set_transient_for_app_window(window);

        if (window) elm_win_raise (window);

        ad.option_window = window;

        ad.event_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_OUT, focus_out_cb, NULL);
    }
}
