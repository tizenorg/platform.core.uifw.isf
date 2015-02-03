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
#include <sclcommon.h>
#include <Ecore.h>

#ifdef WAYLAND
#include <Ecore_Wayland.h>
#else
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <Ecore_X.h>
#endif

#include <vconf.h>
#include <vconf-keys.h>
#include <glib.h>
#include <Elementary.h>
#include <efl_extension.h>

#include "option.h"
#include "languages.h"

#undef LOG_TAG
#define LOG_TAG "ISE_DEFAULT"
#include <dlog.h>

using namespace scl;

#define CONFIG_AUTO_CAPITALISE_VAL      ISE_NAME"/auto_capitalise_val"
#define CONFIG_AUTO_PUNCTUATE_VAL       ISE_NAME"/auto_punctuate_val"
#define CONFIG_KEY_SOUNDS_VAL           ISE_NAME"/key_sounds_val"
#define CONFIG_KEY_VIBRATION_VAL        ISE_NAME"/key_vibration_val"
#define CONFIG_KEY_CHARACTER_PRE_VAL    ISE_NAME"/key_character_pre_val"

#define VCONFKEY_AUTOCAPITAL_ALLOW_BOOL "file/private/isf/autocapital_allow"
#define VCONFKEY_AUTOPERIOD_ALLOW_BOOL  "file/private/isf/autoperiod_allow"


static ISELanguageManager _language_manager;

#define OPTION_MAX_LANGUAGES 255

enum SETTING_ITEM_ID {
    SETTING_ITEM_ID_INPUT_LANGUAGE_TITLE,
    SETTING_ITEM_ID_CUR_LANGUAGE,
    SETTING_ITEM_ID_LANGUAGE,
    SETTING_ITEM_ID_SMART_INPUT_TITLE,
    SETTING_ITEM_ID_AUTO_CAPITALISE,
    SETTING_ITEM_ID_AUTO_PUNCTUATE,
    SETTING_ITEM_ID_KEY_TAP_TITLE,
    SETTING_ITEM_ID_SOUND,
    SETTING_ITEM_ID_VIBRATION,
    SETTING_ITEM_ID_CHARACTER_PRE,

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
        genlist = NULL;
        lang_popup = NULL;
        back_button = NULL;

        itc_main_text_only = NULL;

        itc_language_subitems = NULL;

        languages_item = NULL;

        memset(language_item, 0x00, sizeof(language_item));
        memset(rdg_language, 0x00, sizeof(rdg_language));

        itc_main_text_radio = NULL;
        itc_main_sub_text_radio = NULL;
        itc_group_title = NULL;
    }
    Evas_Object *option_window;
    Evas_Object *naviframe;
    Evas_Object *genlist;
    Evas_Object *lang_popup;
    Evas_Object *back_button;

    Elm_Genlist_Item_Class *itc_main_text_only;

    Elm_Genlist_Item_Class *itc_language_subitems;

    Elm_Object_Item *languages_item;

    Elm_Object_Item *language_item[OPTION_MAX_LANGUAGES];
    Evas_Object *rdg_language[OPTION_MAX_LANGUAGES];

    Elm_Genlist_Item_Class *itc_main_text_radio;
    Elm_Genlist_Item_Class *itc_main_sub_text_radio;
    Elm_Genlist_Item_Class *itc_group_title;
};

static OPTION_ELEMENTS option_elements[OPTION_WINDOW_TYPE_MAX];
extern CONFIG_VALUES g_config_values;
extern CSCLCore g_core;
extern CSCLUI *g_ui;

//static Evas_Object* create_main_window();
static Evas_Object* create_option_language_view(Evas_Object *naviframe);

/* This function is called by setup_module.cpp : create_ise_setup_eo() also */
Evas_Object* create_option_main_view(Evas_Object *parent, Evas_Object *naviframe);

static void language_selected(void *data, Evas_Object *obj, void *event_info);

static void navi_back_cb(void *data, Evas_Object *obj, void *event_info);

static void check_autocapitalise_change_callback(void *data, Evas_Object *obj, void *event_info);
static void check_autopunctuate_change_callback(void *data, Evas_Object *obj, void *event_info);
static void check_sound_change_callback(void *data, Evas_Object *obj, void *event_info);
static void check_vibration_change_callback(void *data, Evas_Object *obj, void *event_info);
static void check_character_pre_change_callback(void *data, Evas_Object *obj, void *event_info);

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

SCLOptionWindowType find_option_window_type(Evas_Object *obj)
{
    for (int loop = 0;loop < OPTION_WINDOW_TYPE_MAX;loop++) {
        if (option_elements[loop].option_window == obj ||
            option_elements[loop].naviframe == obj ||
            option_elements[loop].genlist == obj ||
            option_elements[loop].lang_popup == obj ||
            option_elements[loop].back_button == obj) {
                return static_cast<SCLOptionWindowType>(loop);
        }
    }
    return OPTION_WINDOW_TYPE_MAX;
}

static char *_main_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
    ITEMDATA *item_data = (ITEMDATA*)data;
    if (item_data) {
        if (!strcmp(part, "elm.text.main.left.top") ||
            !strcmp(part, "elm.text.main.left") ||
            !strcmp(part, "elm.text.main") ||
            !strcmp(part, "elm.text") ||
            !strcmp(part, "elm.text.1")) {
            return strdup(item_data->main_text);
        }
        if (!strcmp(part, "elm.text.sub.left.bottom") ||
            !strcmp(part, "elm.text.sub") ||
            !strcmp(part, "elm.text.2")) {
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
        if (!strcmp(part, "elm.icon.right")) {
        }
    }

    return item;
}

static void _main_gl_sel(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *item = (Elm_Object_Item*)event_info;

    int id = (int)reinterpret_cast<long>(data);

    switch (id) {
        case SETTING_ITEM_ID_LANGUAGE: {
            SCLOptionWindowType type = find_option_window_type(obj);
            create_option_language_view(option_elements[type].naviframe);
        }
        break;
        case SETTING_ITEM_ID_AUTO_CAPITALISE: {
            if (item) {
                elm_genlist_item_selected_set(item, EINA_FALSE);
                // Update check button
                Evas_Object *ck = elm_object_item_part_content_get(item, "elm.icon.right");
                evas_object_data_set(ck, "parent_genlist", obj);
                Eina_Bool state = elm_check_state_get(ck);
                elm_check_state_set(ck, !state);
                check_autocapitalise_change_callback((void *)(!state), NULL, NULL);
            }
        break;
        }
        case SETTING_ITEM_ID_AUTO_PUNCTUATE: {
            if (item) {
                elm_genlist_item_selected_set(item, EINA_FALSE);
                // Update check button
                Evas_Object *ck = elm_object_item_part_content_get(item, "elm.icon.right");
                evas_object_data_set(ck, "parent_genlist", obj);
                Eina_Bool state = elm_check_state_get(ck);
                elm_check_state_set(ck, !state);
                check_autopunctuate_change_callback((void *)(!state), NULL, NULL);
            }
        break;
        }
        case SETTING_ITEM_ID_SOUND: {
            if (item) {
                elm_genlist_item_selected_set(item, EINA_FALSE);
                // Update check button
                Evas_Object *ck = elm_object_item_part_content_get(item, "elm.icon.right");
                evas_object_data_set(ck, "parent_genlist", obj);
                Eina_Bool state = elm_check_state_get(ck);
                elm_check_state_set(ck, !state);
                check_sound_change_callback((void *)(!state), NULL, NULL);
            }
        break;
        }
        case SETTING_ITEM_ID_VIBRATION: {
            if (item) {
                elm_genlist_item_selected_set(item, EINA_FALSE);
                // Update check button
                Evas_Object *ck = elm_object_item_part_content_get(item, "elm.icon.right");
                evas_object_data_set(ck, "parent_genlist", obj);
                Eina_Bool state = elm_check_state_get(ck);
                elm_check_state_set(ck, !state);
                check_vibration_change_callback((void *)(!state), NULL, NULL);
            }
        break;
        }
        case SETTING_ITEM_ID_CHARACTER_PRE: {
            if (item) {
                elm_genlist_item_selected_set(item, EINA_FALSE);
                // Update check button
                Evas_Object *ck = elm_object_item_part_content_get(item, "elm.icon.right");
                evas_object_data_set(ck, "parent_genlist", obj);
                Eina_Bool state = elm_check_state_get(ck);
                elm_check_state_set(ck, !state);
                check_character_pre_change_callback((void *)(!state), NULL, NULL);
            }
        break;
        }
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

static char *_language_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
    ITEMDATA *item_data = (ITEMDATA*)data;
    if (item_data) {
        if (!strcmp(part, "elm.text.main.left.top") ||
            !strcmp(part, "elm.text.main.left") ||
            !strcmp(part, "elm.text.main") ||
            !strcmp(part, "elm.text") ||
            !strcmp(part, "elm.text.1")) {
            return strdup(item_data->main_text);
        }
        if (!strcmp(part, "elm.text.sub.left.bottom") ||
            !strcmp(part, "elm.text.2")) {
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
        if (!strcmp(part, "elm.icon.right") ||
            !strcmp(part, "elm.swallow.end")) {
            if (item_data->mode >= 0 && item_data->mode < OPTION_MAX_LANGUAGES) {
                LANGUAGE_INFO *info = _language_manager.get_language_info(item_data->mode);
                if (info) {
                    Evas_Object *ck = elm_check_add(obj);
                    elm_object_style_set(ck, "default/genlist");
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
        Evas_Object *ck = elm_object_item_part_content_get(item, "elm.icon.right");
        if (ck) {
            evas_object_data_set(ck, "parent_genlist", obj);
            Eina_Bool state = elm_check_state_get(ck);
            elm_check_state_set(ck, !state);
            language_selected(item_data, ck, NULL);
        }
    }
}

static void check_autocapitalise_change_callback(void *data, Evas_Object *obj, void *event_info)
{
    Eina_Bool state = EINA_FALSE;
    if(obj) {
        state = elm_check_state_get(obj);
    } else {
        state = (int)(data);
    }
    g_core.config_write_int(CONFIG_AUTO_CAPITALISE_VAL, state);
    if (vconf_set_bool (VCONFKEY_AUTOCAPITAL_ALLOW_BOOL, state) == -1)
        LOGE("Failed to set vconf autocapital");
}

static void check_autopunctuate_change_callback(void *data, Evas_Object *obj, void *event_info)
{
    Eina_Bool state = EINA_FALSE;
    if(obj) {
        state = elm_check_state_get(obj);
    } else {
        state = (int)(data);
    }
    g_core.config_write_int(CONFIG_AUTO_PUNCTUATE_VAL, state);
    if (vconf_set_bool (VCONFKEY_AUTOPERIOD_ALLOW_BOOL, state) == -1)
        LOGE("Failed to set vconf autoperiod");
}

static void check_sound_change_callback(void *data, Evas_Object *obj, void *event_info)
{
    Eina_Bool state = EINA_FALSE;
    if(obj) {
        state = elm_check_state_get(obj);
    } else {
        state = (int)(data);
    }
    g_core.config_write_int(CONFIG_KEY_SOUNDS_VAL, state);
    g_ui->enable_sound (state);
}

static void check_vibration_change_callback(void *data, Evas_Object *obj, void *event_info)
{
    Eina_Bool state = EINA_FALSE;
    if(obj) {
        state = elm_check_state_get(obj);
    } else {
        state = (int)(data);
    }
    g_core.config_write_int(CONFIG_KEY_VIBRATION_VAL, state);
    g_ui->enable_vibration (state);
}

static void check_character_pre_change_callback(void *data, Evas_Object *obj, void *event_info)
{
    Eina_Bool state = EINA_FALSE;
    if(obj) {
        state = elm_check_state_get(obj);
    } else {
        state = (int)(data);
    }
    g_core.config_write_int(CONFIG_KEY_CHARACTER_PRE_VAL, state);
    g_ui->enable_magnifier (state);
}

static Evas_Object *_main_radio_gl_content_get(void *data, Evas_Object *obj, const char *part)
{
    Evas_Object *item = NULL;
    if(NULL != data) {
        ITEMDATA *item_data = (ITEMDATA *)data;
        if (!strcmp(part, "elm.icon.right")) {
            switch (item_data->mode)
            {
                case SETTING_ITEM_ID_AUTO_CAPITALISE:
                    {
                        item = elm_check_add(obj);
                        elm_object_style_set(item, "default/genlist");
                        sclint check_enable = 0;
                        g_core.config_reload();
                        g_core.config_read_int(CONFIG_AUTO_CAPITALISE_VAL, check_enable);
                        elm_check_state_set(item, check_enable);
                        evas_object_propagate_events_set(item, EINA_FALSE);
                        evas_object_smart_callback_add(item, "changed", check_autocapitalise_change_callback, (void*)(item_data->mode));
                        evas_object_show(item);
                        break;
                    }
                case SETTING_ITEM_ID_AUTO_PUNCTUATE:
                    {
                        item = elm_check_add(obj);
                        elm_object_style_set(item, "default/genlist");
                        sclint check_enable = 0;
                        g_core.config_reload();
                        g_core.config_read_int(CONFIG_AUTO_PUNCTUATE_VAL, check_enable);
                        elm_check_state_set(item, check_enable);
                        evas_object_propagate_events_set(item, EINA_FALSE);
                        evas_object_smart_callback_add(item, "changed", check_autopunctuate_change_callback, (void*)(item_data->mode));
                        evas_object_show(item);
                        break;
                   }
                case SETTING_ITEM_ID_SOUND:
                   {
                        item = elm_check_add(obj);
                        elm_object_style_set(item, "default/genlist");
                        sclint check_enable = 0;
                        g_core.config_reload();
                        g_core.config_read_int(CONFIG_KEY_SOUNDS_VAL, check_enable);
                        elm_check_state_set(item, check_enable);
                        evas_object_propagate_events_set(item, EINA_FALSE);
                        evas_object_smart_callback_add(item, "changed", check_sound_change_callback, (void*)(item_data->mode));
                        evas_object_show(item);
                        break;
                   }
                case SETTING_ITEM_ID_VIBRATION:
                   {
                        item = elm_check_add(obj);
                        elm_object_style_set(item, "default/genlist");
                        sclint check_enable = 0;
                        g_core.config_reload();
                        g_core.config_read_int(CONFIG_KEY_VIBRATION_VAL, check_enable);
                        elm_check_state_set(item, check_enable);
                        evas_object_propagate_events_set(item, EINA_FALSE);
                        evas_object_smart_callback_add(item, "changed", check_vibration_change_callback, (void*)(item_data->mode));
                        evas_object_show(item);
                        break;
                   }
                case SETTING_ITEM_ID_CHARACTER_PRE:
                   {
                        item = elm_check_add(obj);
                        elm_object_style_set(item, "default/genlist");
                        sclint check_enable = 0;
                        g_core.config_reload();
                        g_core.config_read_int(CONFIG_KEY_CHARACTER_PRE_VAL, check_enable);
                        elm_check_state_set(item, check_enable);
                        evas_object_propagate_events_set(item, EINA_FALSE);
                        evas_object_smart_callback_add(item, "changed", check_character_pre_change_callback, (void*)(item_data->mode));
                        evas_object_show(item);
                        break;
                   }
            }
        }
    }

    return item;
}

static void
destroy_genlist_item_classes(SCLOptionWindowType type) {
    if (option_elements[type].itc_main_text_only) {
        elm_genlist_item_class_free(option_elements[type].itc_main_text_only);
        option_elements[type].itc_main_text_only = NULL;
    }
    if (option_elements[type].itc_language_subitems) {
        elm_genlist_item_class_free(option_elements[type].itc_language_subitems);
        option_elements[type].itc_language_subitems = NULL;
    }
}

static void
create_genlist_item_classes(SCLOptionWindowType type) {
    option_elements[type].itc_main_text_only = elm_genlist_item_class_new();
    if (option_elements[type].itc_main_text_only) {
        option_elements[type].itc_main_text_only->item_style = "double_label";
        option_elements[type].itc_main_text_only->func.text_get = _main_gl_text_get;
        option_elements[type].itc_main_text_only->func.content_get = _main_gl_content_get;
        option_elements[type].itc_main_text_only->func.state_get = _main_gl_state_get;
        option_elements[type].itc_main_text_only->func.del = _main_gl_del;
    }

    option_elements[type].itc_language_subitems = elm_genlist_item_class_new();
    if (option_elements[type].itc_language_subitems) {
        option_elements[type].itc_language_subitems->item_style = "1line";
        option_elements[type].itc_language_subitems->func.text_get = _language_gl_text_get;
        option_elements[type].itc_language_subitems->func.content_get = _language_gl_content_get;
    }


    option_elements[type].itc_main_text_radio = elm_genlist_item_class_new();
    if (option_elements[type].itc_main_text_radio) {
        option_elements[type].itc_main_text_radio->item_style = "1line";
        option_elements[type].itc_main_text_radio->func.text_get = _main_gl_text_get;
        option_elements[type].itc_main_text_radio->func.content_get = _main_radio_gl_content_get;
        option_elements[type].itc_main_text_radio->func.state_get = _main_gl_state_get;
        option_elements[type].itc_main_text_radio->func.del = _main_gl_del;
    }
    option_elements[type].itc_main_sub_text_radio = elm_genlist_item_class_new();
    if (option_elements[type].itc_main_sub_text_radio) {
        option_elements[type].itc_main_sub_text_radio->item_style = "2line.top";
        option_elements[type].itc_main_sub_text_radio->func.text_get = _main_gl_text_get;
        option_elements[type].itc_main_sub_text_radio->func.content_get = _main_radio_gl_content_get;
        option_elements[type].itc_main_sub_text_radio->func.state_get = _main_gl_state_get;
        option_elements[type].itc_main_sub_text_radio->func.del = _main_gl_del;
    }

    option_elements[type].itc_group_title = elm_genlist_item_class_new();
    if (option_elements[type].itc_group_title) {
        option_elements[type].itc_group_title->item_style = "groupindex";
        option_elements[type].itc_group_title->func.text_get = _main_gl_text_get;
        option_elements[type].itc_group_title->func.content_get = _main_gl_content_get;
        option_elements[type].itc_group_title->func.state_get = _main_gl_state_get;
        option_elements[type].itc_group_title->func.del = _main_gl_del;
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

    SCLOptionWindowType type = find_option_window_type(obj);
    elm_genlist_item_update (option_elements[type].languages_item);

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
    SCLOptionWindowType type = find_option_window_type(obj);
    if (obj) {
        evas_object_smart_callback_del(obj, "timeout", _popup_timeout_cb);
        evas_object_del(obj);
        option_elements[type].lang_popup = NULL;
    }
}


Eina_Bool _pop_cb(void *data, Elm_Object_Item *it)
{
    Evas_Object *naviframe = static_cast<Evas_Object*>(data);
    SCLOptionWindowType type = find_option_window_type(naviframe);
    language_selection_finished_cb(NULL, NULL, NULL);
    if (option_elements[type].lang_popup) {
        _popup_timeout_cb(NULL, option_elements[type].lang_popup, NULL);
    }
    return EINA_TRUE;
}

void
close_option_window(SCLOptionWindowType type)
{
    destroy_genlist_item_classes(type);
    g_core.destroy_option_window(option_elements[type].option_window);
    option_elements[type].option_window = NULL;
}

static void
_naviframe_back_cb (void *data, Evas_Object *obj, void *event_info)
{
    SCLOptionWindowType type = find_option_window_type(obj);
    Elm_Object_Item *top_it = elm_naviframe_top_item_get(obj);
    Elm_Object_Item *bottom_it = elm_naviframe_bottom_item_get(obj);
    if (top_it && bottom_it &&
            (elm_object_item_content_get(top_it) == elm_object_item_content_get(bottom_it))) {
        close_option_window(type);
    } else {
        elm_naviframe_item_pop(obj);
        read_options(obj);
    }
}

Evas_Object* create_option_main_view(Evas_Object *parent, Evas_Object *naviframe, SCLOptionWindowType type)
{
    create_genlist_item_classes(type);

    Evas_Object *genlist = elm_genlist_add(naviframe);
    option_elements[type].genlist = genlist;

    elm_genlist_mode_set (genlist, ELM_LIST_COMPRESS);
    elm_genlist_homogeneous_set (genlist, EINA_TRUE);
    evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_genlist_tree_effect_enabled_set(genlist, EINA_FALSE);

    strncpy(main_itemdata[SETTING_ITEM_ID_INPUT_LANGUAGE_TITLE].main_text, LANGUAGE, ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_INPUT_LANGUAGE_TITLE].mode = SETTING_ITEM_ID_INPUT_LANGUAGE_TITLE;
    elm_genlist_item_append(genlist, option_elements[type].itc_group_title, &main_itemdata[SETTING_ITEM_ID_INPUT_LANGUAGE_TITLE],
                            NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);

   const char *cur_language = _language_manager.get_current_language();
   strncpy(main_itemdata[SETTING_ITEM_ID_CUR_LANGUAGE].main_text, cur_language, ITEM_DATA_STRING_LEN - 1);
   main_itemdata[SETTING_ITEM_ID_CUR_LANGUAGE].mode = SETTING_ITEM_ID_CUR_LANGUAGE;
   elm_genlist_item_append(genlist, option_elements[type].itc_main_text_only, &main_itemdata[SETTING_ITEM_ID_CUR_LANGUAGE],
                            NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel,(void *)(main_itemdata[SETTING_ITEM_ID_CUR_LANGUAGE].mode));

    if (_language_manager.get_languages_num() > 1) {
        std::string languages = compose_selected_languages_string();

        strncpy(main_itemdata[SETTING_ITEM_ID_LANGUAGE].main_text, "Select input language", ITEM_DATA_STRING_LEN - 1);
        strncpy(main_itemdata[SETTING_ITEM_ID_LANGUAGE].sub_text, languages.c_str(), ITEM_DATA_STRING_LEN - 1);
        main_itemdata[SETTING_ITEM_ID_LANGUAGE].mode = SETTING_ITEM_ID_LANGUAGE;
        option_elements[type].languages_item =
            elm_genlist_item_append(genlist, option_elements[type].itc_main_text_only,
            &main_itemdata[SETTING_ITEM_ID_LANGUAGE], NULL, ELM_GENLIST_ITEM_NONE,
            _main_gl_sel, (void*)(main_itemdata[SETTING_ITEM_ID_LANGUAGE].mode));
    }

    strncpy(main_itemdata[SETTING_ITEM_ID_SMART_INPUT_TITLE].main_text, "Smart input functions", ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_SMART_INPUT_TITLE].mode = SETTING_ITEM_ID_SMART_INPUT_TITLE;
    elm_genlist_item_append(genlist, option_elements[type].itc_group_title, &main_itemdata[SETTING_ITEM_ID_SMART_INPUT_TITLE],
                            NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, NULL);

    strncpy(main_itemdata[SETTING_ITEM_ID_AUTO_CAPITALISE].main_text, "Auto capitalise", ITEM_DATA_STRING_LEN - 1);
    strncpy(main_itemdata[SETTING_ITEM_ID_AUTO_CAPITALISE].sub_text, "Capitalise the first letter of each setence automatically",                            ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_AUTO_CAPITALISE].mode = SETTING_ITEM_ID_AUTO_CAPITALISE;
    elm_genlist_item_append(genlist, option_elements[type].itc_main_sub_text_radio,&main_itemdata[SETTING_ITEM_ID_AUTO_CAPITALISE],
                        NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, (void *)(main_itemdata[SETTING_ITEM_ID_AUTO_CAPITALISE].mode));


    strncpy(main_itemdata[SETTING_ITEM_ID_AUTO_PUNCTUATE].main_text, "Auto punctuate", ITEM_DATA_STRING_LEN - 1);
    strncpy(main_itemdata[SETTING_ITEM_ID_AUTO_PUNCTUATE].sub_text,"Automatically insert a full stop by tapping the space bar twice", ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_AUTO_PUNCTUATE].mode = SETTING_ITEM_ID_AUTO_PUNCTUATE;
    elm_genlist_item_append(genlist, option_elements[type].itc_main_sub_text_radio, &main_itemdata[SETTING_ITEM_ID_AUTO_PUNCTUATE],
                          NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, (void *)(main_itemdata[SETTING_ITEM_ID_AUTO_PUNCTUATE].mode));

    strncpy(main_itemdata[SETTING_ITEM_ID_SMART_INPUT_TITLE].main_text, "Key-tap feedback", ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_KEY_TAP_TITLE].mode = SETTING_ITEM_ID_KEY_TAP_TITLE;
    elm_genlist_item_append(genlist, option_elements[type].itc_group_title, &main_itemdata[SETTING_ITEM_ID_KEY_TAP_TITLE],
                            NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, NULL);
    strncpy(main_itemdata[SETTING_ITEM_ID_SOUND].main_text, "Sound", ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_SOUND].mode = SETTING_ITEM_ID_SOUND;
    elm_genlist_item_append(genlist, option_elements[type].itc_main_text_radio, &main_itemdata[SETTING_ITEM_ID_SOUND],
                            NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, (void *)(main_itemdata[SETTING_ITEM_ID_SOUND].mode));
    strncpy(main_itemdata[SETTING_ITEM_ID_VIBRATION].main_text, "Vibration", ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_VIBRATION].mode = SETTING_ITEM_ID_VIBRATION;
    elm_genlist_item_append(genlist, option_elements[type].itc_main_text_radio, &main_itemdata[SETTING_ITEM_ID_VIBRATION],
                            NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, (void *)(main_itemdata[SETTING_ITEM_ID_VIBRATION].mode));
    strncpy(main_itemdata[SETTING_ITEM_ID_CHARACTER_PRE].main_text, "Character preview", ITEM_DATA_STRING_LEN - 1);
    strncpy(main_itemdata[SETTING_ITEM_ID_CHARACTER_PRE].sub_text, "Show a big character bubble when a key on a qwerty keyboard is tapped", ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_AUTO_CAPITALISE].mode = SETTING_ITEM_ID_AUTO_CAPITALISE;
    main_itemdata[SETTING_ITEM_ID_CHARACTER_PRE].mode = SETTING_ITEM_ID_CHARACTER_PRE;
    elm_genlist_item_append(genlist, option_elements[type].itc_main_sub_text_radio, &main_itemdata[SETTING_ITEM_ID_CHARACTER_PRE],
                            NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, (void *)(main_itemdata[SETTING_ITEM_ID_CHARACTER_PRE].mode));

    evas_object_smart_callback_add(genlist, "expanded", _main_gl_exp, genlist);
    evas_object_smart_callback_add(genlist, "contracted", _main_gl_con, genlist);

    return genlist;
}

static Evas_Object* create_option_language_view(Evas_Object *naviframe)
{
    Evas_Object *genlist = elm_genlist_add(naviframe);
    elm_genlist_mode_set (genlist, ELM_LIST_COMPRESS);
    elm_genlist_homogeneous_set (genlist, EINA_TRUE);

    SCLOptionWindowType type = find_option_window_type(naviframe);

    for (unsigned int loop = 0; loop < OPTION_MAX_LANGUAGES && loop < _language_manager.get_languages_num(); loop++)
    {
        LANGUAGE_INFO *info = _language_manager.get_language_info(loop);
        if (info) {
            strncpy(language_itemdata[loop].main_text, info->display_name.c_str(), ITEM_DATA_STRING_LEN - 1);
            language_itemdata[loop].mode = loop;
            option_elements[type].language_item[loop] =
                elm_genlist_item_append(genlist, option_elements[type].itc_language_subitems,
                &language_itemdata[loop], NULL, ELM_GENLIST_ITEM_NONE,
                _language_gl_sel, (void*)(language_itemdata[loop].mode));
        } else {
            option_elements[type].language_item[loop] = NULL;
        }
    }

    evas_object_show(genlist);

    Elm_Object_Item *navi_it = elm_naviframe_item_push(naviframe, LANGUAGE, NULL, NULL, genlist,NULL);

    Evas_Object *back_btn = elm_object_item_part_content_get(navi_it, "elm.swallow.prev_btn");
    evas_object_data_set(genlist, "back_button", back_btn);
    evas_object_smart_callback_add (back_btn, "clicked", language_selection_finished_cb, NULL);
    elm_naviframe_item_pop_cb_set(navi_it, _pop_cb, naviframe);

    return genlist;
}

void read_options(Evas_Object *naviframe)
{
    std::string languages = compose_selected_languages_string();
    strncpy(main_itemdata[SETTING_ITEM_ID_LANGUAGE].sub_text, languages.c_str(), ITEM_DATA_STRING_LEN - 1);

    Elm_Object_Item *top_it;
    top_it = elm_naviframe_top_item_get (naviframe);
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

static void language_selected(void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *genlist = static_cast<Evas_Object*>(evas_object_data_get(obj, "parent_genlist"));
    SCLOptionWindowType type = find_option_window_type(genlist);
    ITEMDATA *item_data = (ITEMDATA*)data;
    if (item_data) {
        LANGUAGE_INFO *info = _language_manager.get_language_info(item_data->mode);
        if (info) {
            info->enabled = elm_check_state_get(obj);
            if (!elm_check_state_get(obj)) {
                if (_language_manager.get_enabled_languages_num() == 0) {
                    info->enabled = TRUE;
                    elm_check_state_set(obj, EINA_TRUE);

                    option_elements[type].lang_popup = elm_popup_add(obj);
                    elm_object_text_set(option_elements[type].lang_popup, MSG_NONE_SELECTED);
                    elm_popup_timeout_set(option_elements[type].lang_popup, 3.0);
                    evas_object_smart_callback_add(option_elements[type].lang_popup, "timeout", _popup_timeout_cb, obj);
                    elm_object_focus_allow_set(option_elements[type].lang_popup, EINA_TRUE);
                    evas_object_show(option_elements[type].lang_popup);
                    eext_object_event_callback_add(option_elements[type].lang_popup, EEXT_CALLBACK_BACK, _popup_timeout_cb, NULL);
                }
            }
        }
    }
}

static void
navi_back_cb(void *data, Evas_Object *obj, void *event_info)
{
    SCLOptionWindowType type = find_option_window_type(obj);
    evas_object_smart_callback_del(obj, "clicked", navi_back_cb);
    close_option_window(type);
}

void
option_window_created(Evas_Object *window, SCLOptionWindowType type)
{
    if (window == NULL) return;

    read_ise_config_values();

    /* To make sure there is no temporary language in the enabled language list */
    _language_manager.set_enabled_languages(g_config_values.enabled_languages);

    set_option_values();

    option_elements[type].option_window = window;
    elm_win_indicator_mode_set(window, ELM_WIN_INDICATOR_SHOW);
    elm_win_indicator_opacity_set(window, ELM_WIN_INDICATOR_OPAQUE);

    Evas_Object *conformant = elm_conformant_add(window);
    evas_object_size_hint_weight_set(conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(conformant, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_win_resize_object_add(window, conformant);
    elm_win_conformant_set(window, EINA_TRUE);
    evas_object_show(conformant);

    /* create header bg */
    Evas_Object *bg = elm_bg_add(conformant);
    elm_object_style_set(bg, "indicator/headerbg");
    elm_object_part_content_set(conformant, "elm.swallow.indicator_bg", bg);
    evas_object_show(bg);

    Evas_Object *naviframe = elm_naviframe_add(conformant);
    elm_naviframe_prev_btn_auto_pushed_set(naviframe, EINA_FALSE);
    eext_object_event_callback_add(naviframe, EEXT_CALLBACK_BACK, _naviframe_back_cb, NULL);
    evas_object_size_hint_weight_set(naviframe, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(naviframe, EVAS_HINT_FILL, EVAS_HINT_FILL);

    Evas_Object *list = create_option_main_view(conformant, naviframe);

    /* add a back button to naviframe */
    Evas_Object *back_btn = elm_button_add(naviframe);
    elm_object_style_set(back_btn, "naviframe/back_btn/default");
    evas_object_smart_callback_add (back_btn, "clicked", navi_back_cb, NULL);
    elm_naviframe_item_push(naviframe, OPTIONS, back_btn, NULL, list, NULL);

    elm_object_content_set(conformant, naviframe);
    evas_object_show(naviframe);
    evas_object_show(window);
}

void
option_window_destroyed(Evas_Object *window)
{
    SCLOptionWindowType type = find_option_window_type(window);
    if (option_elements[type].option_window == window) {
        option_elements[type].option_window = NULL;
        option_elements[type].naviframe = NULL;
        option_elements[type].genlist = NULL;
        option_elements[type].lang_popup = NULL;
        option_elements[type].back_button = NULL;
    }
}
