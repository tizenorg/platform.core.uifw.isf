/*
 * Copyright (c) 2012 - 2015 Samsung Electronics Co., Ltd All Rights Reserved
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


static ISELanguageManager _language_manager;

#define OPTION_MAX_LANGUAGES        255
#define MIN_SELECTED_LANGUAGES      1
#define MAX_SELECTED_LANGUAGES      4

enum SETTING_ITEM_ID {
    SETTING_ITEM_ID_INPUT_LANGUAGE_TITLE,
    SETTING_ITEM_ID_CUR_LANGUAGE,
    SETTING_ITEM_ID_SELECT_INPUT_LANGUAGE,
    SETTING_ITEM_ID_SMART_INPUT_TITLE,
    SETTING_ITEM_ID_AUTO_CAPITALISE,
    SETTING_ITEM_ID_AUTO_PUNCTUATE,
    SETTING_ITEM_ID_KEY_TAP_TITLE,
    SETTING_ITEM_ID_SOUND,
    SETTING_ITEM_ID_VIBRATION,
    SETTING_ITEM_ID_CHARACTER_PRE,
    SETTING_ITEM_ID_MORE_SETTINGS_TITLE,
    SETTING_ITEM_ID_RESET,

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
        memset(selected_language_item, 0x00, sizeof(selected_language_item));

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
    Elm_Object_Item *selected_language_item[OPTION_MAX_LANGUAGES];

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
Evas_Object* create_option_main_view(Evas_Object *parent, Evas_Object *naviframe, SCLOptionWindowType type);

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
    LOGD ("OPTION_WINDOW_TYPE_MAX=%d", OPTION_WINDOW_TYPE_MAX);
    return OPTION_WINDOW_TYPE_MAX;
}

static void reset_settings_popup_response_ok_cb (void *data, Evas_Object *obj, void *event_info)
{
    reset_ise_config_values ();
    _language_manager.set_enabled_languages (g_config_values.enabled_languages);

    Evas_Object *genlist = static_cast<Evas_Object*>(evas_object_data_get (obj, "parent_genlist"));
    SCLOptionWindowType type = find_option_window_type (genlist);
    read_options (option_elements[type].naviframe);

    g_ui->enable_sound (g_config_values.sound_on);
    g_ui->enable_vibration (g_config_values.vibration_on);
    g_ui->enable_magnifier (g_config_values.preview_on);
    vconf_set_bool (VCONFKEY_AUTOCAPITAL_ALLOW_BOOL, g_config_values.auto_capitalise);
    vconf_set_bool (VCONFKEY_AUTOPERIOD_ALLOW_BOOL, g_config_values.auto_punctuate);

    Evas_Object *popup = (Evas_Object *)data;
    if (popup)
        evas_object_del (popup);
}

static void reset_settings_popup_response_cancel_cb (void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *popup = (Evas_Object *)data;
    if (popup)
        evas_object_del (popup);
}

static void reset_settings_popup (void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *popup = elm_popup_add (elm_object_top_widget_get (obj));
    elm_popup_align_set (popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
    eext_object_event_callback_add (popup, EEXT_CALLBACK_BACK, eext_popup_back_cb, NULL);
    evas_object_size_hint_weight_set (popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_object_domain_translatable_text_set (popup, PACKAGE, RESET_SETTINGS_POPUP_TEXT);
    elm_object_domain_translatable_part_text_set (popup, "title,text", PACKAGE, RESET_SETTINGS_POPUP_TITLE_TEXT);

    Evas_Object *btn_cancel = elm_button_add (popup);
    elm_object_style_set (btn_cancel, "popup");
    elm_object_domain_translatable_text_set (btn_cancel, PACKAGE, POPUP_CANCEL_BTN);
    elm_object_part_content_set (popup, "button1", btn_cancel);
    evas_object_smart_callback_add (btn_cancel, "clicked", reset_settings_popup_response_cancel_cb, popup);

    Evas_Object *btn_ok = elm_button_add (popup);
    evas_object_data_set (btn_ok, "parent_genlist", obj);
    elm_object_style_set (btn_ok, "popup");
    elm_object_domain_translatable_text_set (btn_ok, PACKAGE, POPUP_OK_BTN);
    elm_object_part_content_set (popup, "button2", btn_ok);
    evas_object_smart_callback_add (btn_ok, "clicked", reset_settings_popup_response_ok_cb, popup);
    evas_object_show (popup);
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
            !strcmp(part, "elm.text.multiline") ||
            !strcmp(part, "elm.text.sub") ||
            !strcmp(part, "elm.text.2")) {
            if (strlen(item_data->sub_text) > 0)
                return strdup(item_data->sub_text);
        }
    }

    return NULL;
}

static Eina_Bool _update_check_button_state (Elm_Object_Item *item, Evas_Object *obj)
{
    Eina_Bool state = EINA_FALSE;
    if (item && obj) {
        elm_genlist_item_selected_set (item, EINA_FALSE);
        /* Update check button */
        Evas_Object *ck = elm_object_item_part_content_get (item, "elm.swallow.end");
        if (ck == NULL)
            ck = elm_object_item_part_content_get (item, "elm.icon");
        evas_object_data_set (ck, "parent_genlist", obj);
        state = !elm_check_state_get (ck);
        elm_check_state_set (ck, state);
    }
    return state;
}

static void _main_gl_sel(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *item = (Elm_Object_Item*)event_info;

    if (data) {
        int id = (int)reinterpret_cast<long>(data);
        Eina_Bool state = EINA_FALSE;

        switch (id) {
            case SETTING_ITEM_ID_SELECT_INPUT_LANGUAGE: {
                SCLOptionWindowType type = find_option_window_type(obj);
                create_option_language_view(option_elements[type].naviframe);
            }
            break;
            case SETTING_ITEM_ID_AUTO_CAPITALISE: {
                if (item) {
                    state = _update_check_button_state (item, obj);
                    check_autocapitalise_change_callback((void *)(state), NULL, NULL);
                }
            break;
            }
            case SETTING_ITEM_ID_AUTO_PUNCTUATE: {
                if (item) {
                    state = _update_check_button_state (item, obj);
                    check_autopunctuate_change_callback((void *)(state), NULL, NULL);
                }
            break;
            }
            case SETTING_ITEM_ID_SOUND: {
                if (item) {
                    state = _update_check_button_state (item, obj);
                    check_sound_change_callback((void *)(state), NULL, NULL);
                }
            break;
            }
            case SETTING_ITEM_ID_VIBRATION: {
                if (item) {
                    state = _update_check_button_state (item, obj);
                    check_vibration_change_callback((void *)(state), NULL, NULL);
                }
            break;
            }
            case SETTING_ITEM_ID_CHARACTER_PRE: {
                if (item) {
                    state = _update_check_button_state (item, obj);
                    check_character_pre_change_callback((void *)(state), NULL, NULL);
                }
            break;
            }
            case SETTING_ITEM_ID_RESET: {
                LOGD("reset keyboard settings");
                reset_settings_popup (data, obj, event_info);
            }
            break;
            default:
            break;
        }
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
            !strcmp(part, "elm.text.multiline") ||
            !strcmp(part, "elm.text.2")) {
            if (strlen(item_data->sub_text) > 0)
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
            !strcmp(part, "elm.icon") ||
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
        Evas_Object *ck = elm_object_item_part_content_get(item, "elm.swallow.end");
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
        state = (int)reinterpret_cast<long>(data);
    }
    g_config_values.auto_capitalise = state;
    write_ise_config_values ();
    if (vconf_set_bool (VCONFKEY_AUTOCAPITAL_ALLOW_BOOL, state) == -1)
        LOGE("Failed to set vconf autocapital");
}

static void check_autopunctuate_change_callback(void *data, Evas_Object *obj, void *event_info)
{
    Eina_Bool state = EINA_FALSE;
    if(obj) {
        state = elm_check_state_get(obj);
    } else {
        state = (int)reinterpret_cast<long>(data);
    }
    g_config_values.auto_punctuate = state;
    write_ise_config_values ();
    if (vconf_set_bool (VCONFKEY_AUTOPERIOD_ALLOW_BOOL, state) == -1)
        LOGE("Failed to set vconf autoperiod");
}

static void check_sound_change_callback(void *data, Evas_Object *obj, void *event_info)
{
    Eina_Bool state = EINA_FALSE;
    if(obj) {
        state = elm_check_state_get(obj);
    } else {
        state = (int)reinterpret_cast<long>(data);
    }
    g_config_values.sound_on = state;
    write_ise_config_values ();
    g_ui->enable_sound (state);
}

static void check_vibration_change_callback(void *data, Evas_Object *obj, void *event_info)
{
    Eina_Bool state = EINA_FALSE;
    if(obj) {
        state = elm_check_state_get(obj);
    } else {
        state = (int)reinterpret_cast<long>(data);
    }
    g_config_values.vibration_on = state;
    write_ise_config_values ();
    g_ui->enable_vibration (state);
}

static void check_character_pre_change_callback(void *data, Evas_Object *obj, void *event_info)
{
    Eina_Bool state = EINA_FALSE;
    if(obj) {
        state = elm_check_state_get(obj);
    } else {
        state = (int)reinterpret_cast<long>(data);
    }
    g_config_values.preview_on = state;
    write_ise_config_values ();
    g_ui->enable_magnifier (state);
}

static Evas_Object *_create_check_button (Evas_Object *parent, sclboolean state)
{
    Evas_Object *ck = NULL;
    if (parent) {
        ck = elm_check_add (parent);
        elm_object_style_set (ck, "on&off");
        elm_check_state_set (ck, state);
        evas_object_propagate_events_set (ck, EINA_FALSE);
        evas_object_show (ck);
    }

    return ck;
}

static Evas_Object *_main_radio_gl_content_get(void *data, Evas_Object *obj, const char *part)
{
    Evas_Object *ck = NULL;
    if(NULL != data) {
        ITEMDATA *item_data = (ITEMDATA *)data;
        if (!strcmp(part, "elm.swallow.end") ||
            !strcmp(part, "elm.icon.right") ||
            !strcmp(part, "elm.icon")) {
            switch (item_data->mode) {
                case SETTING_ITEM_ID_AUTO_CAPITALISE:
                    ck = _create_check_button (obj, g_config_values.auto_capitalise);
                    evas_object_smart_callback_add (ck, "changed", check_autocapitalise_change_callback, (void*)(item_data->mode));
                    break;
                case SETTING_ITEM_ID_AUTO_PUNCTUATE:
                    ck = _create_check_button (obj, g_config_values.auto_punctuate);
                    evas_object_smart_callback_add (ck, "changed", check_autopunctuate_change_callback, (void*)(item_data->mode));
                    break;
                case SETTING_ITEM_ID_SOUND:
                    ck = _create_check_button (obj, g_config_values.sound_on);
                    evas_object_smart_callback_add (ck, "changed", check_sound_change_callback, (void*)(item_data->mode));
                    break;
                case SETTING_ITEM_ID_VIBRATION:
                    ck = _create_check_button (obj, g_config_values.vibration_on);
                    evas_object_smart_callback_add (ck, "changed", check_vibration_change_callback, (void*)(item_data->mode));
                    break;
                case SETTING_ITEM_ID_CHARACTER_PRE:
                    ck = _create_check_button (obj, g_config_values.preview_on);
                    evas_object_smart_callback_add (ck, "changed", check_character_pre_change_callback, (void*)(item_data->mode));
                    break;
            }
        }
    }

    return ck;
}

static void destroy_genlist_item_classes(SCLOptionWindowType type)
{
    if (option_elements[type].itc_main_text_only) {
        elm_genlist_item_class_free(option_elements[type].itc_main_text_only);
        option_elements[type].itc_main_text_only = NULL;
    }
    if (option_elements[type].itc_language_subitems) {
        elm_genlist_item_class_free(option_elements[type].itc_language_subitems);
        option_elements[type].itc_language_subitems = NULL;
    }
    if (option_elements[type].itc_main_text_radio) {
        elm_genlist_item_class_free(option_elements[type].itc_main_text_radio);
        option_elements[type].itc_main_text_radio = NULL;
    }
    if (option_elements[type].itc_main_sub_text_radio) {
        elm_genlist_item_class_free(option_elements[type].itc_main_sub_text_radio);
        option_elements[type].itc_main_sub_text_radio = NULL;
    }
    if (option_elements[type].itc_group_title) {
        elm_genlist_item_class_free(option_elements[type].itc_group_title);
        option_elements[type].itc_group_title = NULL;
    }
}

static void create_genlist_item_classes(SCLOptionWindowType type)
{
    option_elements[type].itc_main_text_only = elm_genlist_item_class_new();
    if (option_elements[type].itc_main_text_only) {
        option_elements[type].itc_main_text_only->item_style = "type1";
        option_elements[type].itc_main_text_only->func.text_get = _main_gl_text_get;
        option_elements[type].itc_main_text_only->func.content_get = NULL;
        option_elements[type].itc_main_text_only->func.state_get = _main_gl_state_get;
        option_elements[type].itc_main_text_only->func.del = _main_gl_del;
    }

    option_elements[type].itc_language_subitems = elm_genlist_item_class_new();
    if (option_elements[type].itc_language_subitems) {
        option_elements[type].itc_language_subitems->item_style = "type1";
        option_elements[type].itc_language_subitems->func.text_get = _language_gl_text_get;
        option_elements[type].itc_language_subitems->func.content_get = _language_gl_content_get;
        option_elements[type].itc_language_subitems->func.state_get = _main_gl_state_get;
        option_elements[type].itc_language_subitems->func.del = _main_gl_del;
    }

    option_elements[type].itc_main_text_radio = elm_genlist_item_class_new();
    if (option_elements[type].itc_main_text_radio) {
        option_elements[type].itc_main_text_radio->item_style = "type1";
        option_elements[type].itc_main_text_radio->func.text_get = _main_gl_text_get;
        option_elements[type].itc_main_text_radio->func.content_get = _main_radio_gl_content_get;
        option_elements[type].itc_main_text_radio->func.state_get = _main_gl_state_get;
        option_elements[type].itc_main_text_radio->func.del = _main_gl_del;
    }

    option_elements[type].itc_main_sub_text_radio = elm_genlist_item_class_new();
    if (option_elements[type].itc_main_sub_text_radio) {
        option_elements[type].itc_main_sub_text_radio->item_style = "type1";//"multiline_sub.main.1icon";
        option_elements[type].itc_main_sub_text_radio->func.text_get = _main_gl_text_get;
        option_elements[type].itc_main_sub_text_radio->func.content_get = _main_radio_gl_content_get;
        option_elements[type].itc_main_sub_text_radio->func.state_get = _main_gl_state_get;
        option_elements[type].itc_main_sub_text_radio->func.del = _main_gl_del;
    }

    option_elements[type].itc_group_title = elm_genlist_item_class_new();
    if (option_elements[type].itc_group_title) {
        option_elements[type].itc_group_title->item_style = "group_index";
        option_elements[type].itc_group_title->func.text_get = _main_gl_text_get;
        option_elements[type].itc_group_title->func.content_get = NULL;
        option_elements[type].itc_group_title->func.state_get = _main_gl_state_get;
        option_elements[type].itc_group_title->func.del = _main_gl_del;
    }
}

static std::string compose_selected_languages_string(void)
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

static void language_selection_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
    if (_language_manager.get_enabled_languages_num() == 0) {
        read_ise_config_values();
        _language_manager.set_enabled_languages(g_config_values.enabled_languages);
    }

    SCLOptionWindowType type = find_option_window_type(obj);
    elm_genlist_item_update (option_elements[type].languages_item);

    if (obj) {
        evas_object_smart_callback_del(obj, "clicked", language_selection_finished_cb);
        Evas_Object *naviframe = (Evas_Object *)data;
        if (naviframe) {
            elm_naviframe_item_pop (naviframe);
            read_options (naviframe);
        }
    }

    sclboolean selected_language_found = FALSE;
    std::vector<std::string> enabled_languages;

    for (unsigned int loop = 0;loop < OPTION_MAX_LANGUAGES && loop < _language_manager.get_languages_num();loop++) {
        LANGUAGE_INFO *info = _language_manager.get_language_info(loop);
        if (info) {
            if (info->enabled) {
                enabled_languages.push_back(info->name);
                LOGD ("Enabled language:%s", info->name.c_str ());
                if (info->name.compare(g_config_values.selected_language) == 0) {
                    selected_language_found = TRUE;
                }
            }
        }
    }
    if (enabled_languages.size() > 0) {
        g_config_values.enabled_languages = enabled_languages;
        LOGD ("Enabled languages size:%d", g_config_values.enabled_languages.size ());
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

static Eina_Bool _pop_cb(void *data, Elm_Object_Item *it)
{
    Evas_Object *naviframe = static_cast<Evas_Object*>(data);
    SCLOptionWindowType type = find_option_window_type(naviframe);
    language_selection_finished_cb(NULL, NULL, NULL);
    if (option_elements[type].lang_popup) {
        _popup_timeout_cb(NULL, option_elements[type].lang_popup, NULL);
    }
    return EINA_TRUE;
}

static void close_option_window(SCLOptionWindowType type)
{
    destroy_genlist_item_classes(type);
    g_core.destroy_option_window(option_elements[type].option_window);
    option_elements[type].option_window = NULL;
}

static void _naviframe_back_cb (void *data, Evas_Object *obj, void *event_info)
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

    Elm_Object_Item *item = NULL;
    Evas_Object *genlist = elm_genlist_add(naviframe);
    option_elements[type].genlist = genlist;

    elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
    evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_genlist_tree_effect_enabled_set(genlist, EINA_FALSE);

    /* Input languages */
    strncpy(main_itemdata[SETTING_ITEM_ID_INPUT_LANGUAGE_TITLE].main_text, LANGUAGE, ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_INPUT_LANGUAGE_TITLE].mode = SETTING_ITEM_ID_INPUT_LANGUAGE_TITLE;
    item = elm_genlist_item_append(genlist, option_elements[type].itc_group_title, &main_itemdata[SETTING_ITEM_ID_INPUT_LANGUAGE_TITLE],
                                   NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
    elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

    for (unsigned int loop = 0; loop < OPTION_MAX_LANGUAGES && loop < _language_manager.get_languages_num (); loop++)
    {
        LANGUAGE_INFO *info = _language_manager.get_language_info (loop);
        if (info && info->enabled) {
            strncpy (language_itemdata[loop].main_text, info->display_name.c_str (), ITEM_DATA_STRING_LEN - 1);
            option_elements[type].selected_language_item[loop] =
                elm_genlist_item_append (genlist, option_elements[type].itc_main_text_only, &language_itemdata[loop],
                                        NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, NULL);
        } else {
            option_elements[type].selected_language_item[loop] = NULL;
        }
    }

    if (_language_manager.get_languages_num() > 1) {
        strncpy(main_itemdata[SETTING_ITEM_ID_SELECT_INPUT_LANGUAGE].main_text, SELECT_LANGUAGES, ITEM_DATA_STRING_LEN - 1);
        //std::string languages = compose_selected_languages_string();
        //strncpy(main_itemdata[SETTING_ITEM_ID_SELECT_INPUT_LANGUAGE].sub_text, languages.c_str(), ITEM_DATA_STRING_LEN - 1);
        main_itemdata[SETTING_ITEM_ID_SELECT_INPUT_LANGUAGE].mode = SETTING_ITEM_ID_SELECT_INPUT_LANGUAGE;
        option_elements[type].languages_item =
            elm_genlist_item_append(genlist, option_elements[type].itc_main_text_only, &main_itemdata[SETTING_ITEM_ID_SELECT_INPUT_LANGUAGE],
                                    NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, (void*)(main_itemdata[SETTING_ITEM_ID_SELECT_INPUT_LANGUAGE].mode));
    }

    /* Smart input functions */
    strncpy(main_itemdata[SETTING_ITEM_ID_SMART_INPUT_TITLE].main_text, SMART_FUNCTIONS, ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_SMART_INPUT_TITLE].mode = SETTING_ITEM_ID_SMART_INPUT_TITLE;
    item = elm_genlist_item_append(genlist, option_elements[type].itc_group_title, &main_itemdata[SETTING_ITEM_ID_SMART_INPUT_TITLE],
                                   NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, NULL);
    elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

    strncpy(main_itemdata[SETTING_ITEM_ID_AUTO_CAPITALISE].main_text, AUTO_CAPITALISE, ITEM_DATA_STRING_LEN - 1);
    strncpy(main_itemdata[SETTING_ITEM_ID_AUTO_CAPITALISE].sub_text, CAPITALISE_DESC, ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_AUTO_CAPITALISE].mode = SETTING_ITEM_ID_AUTO_CAPITALISE;
    elm_genlist_item_append(genlist, option_elements[type].itc_main_sub_text_radio, &main_itemdata[SETTING_ITEM_ID_AUTO_CAPITALISE],
                        NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, (void *)(main_itemdata[SETTING_ITEM_ID_AUTO_CAPITALISE].mode));

    strncpy(main_itemdata[SETTING_ITEM_ID_AUTO_PUNCTUATE].main_text, AUTO_PUNCTUATE, ITEM_DATA_STRING_LEN - 1);
    strncpy(main_itemdata[SETTING_ITEM_ID_AUTO_PUNCTUATE].sub_text, PUNCTUATE_DESC, ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_AUTO_PUNCTUATE].mode = SETTING_ITEM_ID_AUTO_PUNCTUATE;
    elm_genlist_item_append(genlist, option_elements[type].itc_main_sub_text_radio, &main_itemdata[SETTING_ITEM_ID_AUTO_PUNCTUATE],
                          NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, (void *)(main_itemdata[SETTING_ITEM_ID_AUTO_PUNCTUATE].mode));

    /* Key-tap feedback */
    strncpy(main_itemdata[SETTING_ITEM_ID_KEY_TAP_TITLE].main_text, KEY_FEEDBACK, ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_KEY_TAP_TITLE].mode = SETTING_ITEM_ID_KEY_TAP_TITLE;
    item = elm_genlist_item_append(genlist, option_elements[type].itc_group_title, &main_itemdata[SETTING_ITEM_ID_KEY_TAP_TITLE],
                                   NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, NULL);
    elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

    strncpy(main_itemdata[SETTING_ITEM_ID_SOUND].main_text, SOUND, ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_SOUND].mode = SETTING_ITEM_ID_SOUND;
    elm_genlist_item_append(genlist, option_elements[type].itc_main_text_radio, &main_itemdata[SETTING_ITEM_ID_SOUND],
                            NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, (void *)(main_itemdata[SETTING_ITEM_ID_SOUND].mode));
    strncpy(main_itemdata[SETTING_ITEM_ID_VIBRATION].main_text, VIBRATION, ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_VIBRATION].mode = SETTING_ITEM_ID_VIBRATION;
    elm_genlist_item_append(genlist, option_elements[type].itc_main_text_radio, &main_itemdata[SETTING_ITEM_ID_VIBRATION],
                            NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, (void *)(main_itemdata[SETTING_ITEM_ID_VIBRATION].mode));
    strncpy(main_itemdata[SETTING_ITEM_ID_CHARACTER_PRE].main_text, CHARACTER_PREVIEW, ITEM_DATA_STRING_LEN - 1);
    strncpy(main_itemdata[SETTING_ITEM_ID_CHARACTER_PRE].sub_text, PREVIEW_DESC, ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_CHARACTER_PRE].mode = SETTING_ITEM_ID_CHARACTER_PRE;
    elm_genlist_item_append(genlist, option_elements[type].itc_main_sub_text_radio, &main_itemdata[SETTING_ITEM_ID_CHARACTER_PRE],
                            NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, (void *)(main_itemdata[SETTING_ITEM_ID_CHARACTER_PRE].mode));

    /* More settings */
    strncpy(main_itemdata[SETTING_ITEM_ID_MORE_SETTINGS_TITLE].main_text, MORE_SETTINGS, ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_MORE_SETTINGS_TITLE].mode = SETTING_ITEM_ID_MORE_SETTINGS_TITLE;
    item = elm_genlist_item_append(genlist, option_elements[type].itc_group_title, &main_itemdata[SETTING_ITEM_ID_MORE_SETTINGS_TITLE],
                                   NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, NULL);
    elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

    strncpy(main_itemdata[SETTING_ITEM_ID_RESET].main_text, RESET, ITEM_DATA_STRING_LEN - 1);
    main_itemdata[SETTING_ITEM_ID_RESET].mode = SETTING_ITEM_ID_RESET;
    elm_genlist_item_append(genlist, option_elements[type].itc_main_text_only, &main_itemdata[SETTING_ITEM_ID_RESET],
                        NULL, ELM_GENLIST_ITEM_NONE, _main_gl_sel, (void *)(main_itemdata[SETTING_ITEM_ID_RESET].mode));

    evas_object_smart_callback_add(genlist, "expanded", _main_gl_exp, genlist);
    evas_object_smart_callback_add(genlist, "contracted", _main_gl_con, genlist);

    return genlist;
}

static Evas_Object* create_option_language_view(Evas_Object *naviframe)
{
    Evas_Object *genlist = elm_genlist_add(naviframe);
    elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
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

    Evas_Object *back_button = elm_button_add (naviframe);
    elm_object_style_set (back_button, "naviframe/back_btn/default");
    evas_object_smart_callback_add (back_button, "clicked", language_selection_finished_cb, naviframe);
    Elm_Object_Item *navi_it = elm_naviframe_item_push (naviframe, LANGUAGE, back_button, NULL, genlist, NULL);
    elm_naviframe_item_pop_cb_set (navi_it, _pop_cb, naviframe);

    return genlist;
}

void read_options(Evas_Object *naviframe)
{
    SCLOptionWindowType type = find_option_window_type(naviframe);
    for (unsigned int loop = 0; loop < OPTION_MAX_LANGUAGES && loop < _language_manager.get_languages_num (); loop++)
    {
        if (option_elements[type].selected_language_item[loop] != NULL) {
            elm_object_item_del (option_elements[type].selected_language_item[loop]);
            option_elements[type].selected_language_item[loop] = NULL;
        }

        LANGUAGE_INFO *info = _language_manager.get_language_info (loop);
        if (info && info->enabled) {
            strncpy (language_itemdata[loop].main_text, info->display_name.c_str (), ITEM_DATA_STRING_LEN - 1);
            option_elements[type].selected_language_item[loop] =
                elm_genlist_item_insert_before (option_elements[type].genlist, option_elements[type].itc_main_text_only, &language_itemdata[loop],
                                        NULL, option_elements[type].languages_item, ELM_GENLIST_ITEM_NONE, _main_gl_sel, NULL);
        }
    }

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
}

static void language_view_popup_show (Evas_Object *obj, SCLOptionWindowType type, const char *content_text, const char *btn_text)
{
    if (content_text == NULL)
        return;

    Evas_Object *top_level = elm_object_top_widget_get (obj);
    option_elements[type].lang_popup = elm_popup_add (top_level);
    elm_object_text_set (option_elements[type].lang_popup, content_text);
    elm_popup_align_set (option_elements[type].lang_popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
    elm_popup_timeout_set (option_elements[type].lang_popup, 3.0);
    evas_object_smart_callback_add (option_elements[type].lang_popup, "timeout", _popup_timeout_cb, obj);
    elm_object_focus_allow_set (option_elements[type].lang_popup, EINA_TRUE);
    evas_object_show (option_elements[type].lang_popup);
    evas_object_raise (option_elements[type].lang_popup);
    eext_object_event_callback_add (option_elements[type].lang_popup, EEXT_CALLBACK_BACK, _popup_timeout_cb, NULL);
}

static void language_selected(void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *genlist = static_cast<Evas_Object*>(evas_object_data_get(obj, "parent_genlist"));
    SCLOptionWindowType type = find_option_window_type(genlist);
    LOGD ("type=%d", type);
    ITEMDATA *item_data = (ITEMDATA*)data;
    if (item_data) {
        LANGUAGE_INFO *info = _language_manager.get_language_info(item_data->mode);
        if (info) {
            info->enabled = elm_check_state_get(obj);
            if (!elm_check_state_get(obj)) {
                if (_language_manager.get_enabled_languages_num() < MIN_SELECTED_LANGUAGES) {
                    info->enabled = TRUE;
                    elm_check_state_set(obj, EINA_TRUE);
                    language_view_popup_show (obj, type, MSG_NONE_SELECTED, NULL);
                }
            } else {
                if (_language_manager.get_enabled_languages_num() > MAX_SELECTED_LANGUAGES) {
                    info->enabled = FALSE;
                    elm_check_state_set(obj, EINA_FALSE);

                    char buf[256] = {0};
                    snprintf(buf, 256, SUPPORTED_MAX_LANGUAGES, MAX_SELECTED_LANGUAGES);
                    language_view_popup_show (obj, type, buf, NULL);
                }
            }
        }
    }
}

static void navi_back_cb(void *data, Evas_Object *obj, void *event_info)
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

    /* Create header bg */
    Evas_Object *bg = elm_bg_add(conformant);
    elm_object_style_set(bg, "indicator/headerbg");
    elm_object_part_content_set(conformant, "elm.swallow.indicator_bg", bg);
    evas_object_show(bg);

    Evas_Object *naviframe = elm_naviframe_add(conformant);
    option_elements[type].naviframe = naviframe;

    elm_naviframe_prev_btn_auto_pushed_set(naviframe, EINA_FALSE);
    eext_object_event_callback_add(naviframe, EEXT_CALLBACK_BACK, _naviframe_back_cb, NULL);
    evas_object_size_hint_weight_set(naviframe, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(naviframe, EVAS_HINT_FILL, EVAS_HINT_FILL);

    Evas_Object *list = create_option_main_view(conformant, naviframe, type);

    /* Add a back button to naviframe */
    Evas_Object *back_button = elm_button_add(naviframe);
    option_elements[type].back_button = back_button;

    elm_object_style_set(back_button, "naviframe/back_btn/default");
    evas_object_smart_callback_add (back_button, "clicked", navi_back_cb, NULL);
    elm_naviframe_item_push(naviframe, OPTIONS, back_button, NULL, list, NULL);

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
