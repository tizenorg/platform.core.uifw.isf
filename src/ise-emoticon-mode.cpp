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

#ifdef SUPPORTS_EMOTICONS

#include <string>
#include <vector>
#include <sstream>
#include <iterator>
#include <Elementary.h>
#include <algorithm>
#include "ise-emoticon-mode.h"
#include "ise.h"
#include "config.h"
#include "sclutils.h"
#include "sclfeedback.h"

#undef LOG_TAG
#define LOG_TAG "ISE_DEFAULT"
#include <dlog.h>

#define EVAS_CANDIDATE_LAYER    32000

#ifdef _TV
#define ISE_HEIGHT_PORT         398
#define ISE_HEIGHT_LAND         398
#else
#define ISE_HEIGHT_PORT         442
#define ISE_HEIGHT_LAND         318
#endif

#define EMOTICON_DIR LAYOUTDIR"/emoticons/"
#define EMOTICON_EDJ_FILE_PATH LAYOUTDIR"/sdk/edc/layout_keypad.edj"
#define CUSTOM_GENGRID_EDJ_FILE_PATH LAYOUTDIR"/sdk/edc/customised_gengrid.edj"

#define EMOTICON_EDJ_GROUP_PORT_CANDIDATE_ON "emoticon.main.portrait.candidate.on"
#define EMOTICON_EDJ_GROUP_LAND_CANDIDATE_ON "emoticon.main.landscape.candidate.on"

#define EMOTICON_EDJ_GROUP_PORT_CANDIDATE_OFF "emoticon.main.portrait.candidate.off"
#define EMOTICON_EDJ_GROUP_LAND_CANDIDATE_OFF "emoticon.main.landscape.candidate.off"

#define EMOTICON_GENGRID_ITEM_STYLE_PORT "ise/customized_default_style_port"
#define EMOTICON_GENGRID_ITEM_STYLE_LAND "ise/customized_default_style_land"

#define EMOTICON_GENGRID_ITEM_STYLE_PORT2 "ise/customized_default_style_port2"
#define EMOTICON_GENGRID_ITEM_STYLE_LAND2 "ise/customized_default_style_land2"

#define IND_NUM 20

#define EMOTICON_ICON_WIDTH_PORT 30
#define EMOTICON_ICON_HEIGHT_PORT 30

#define EMOTICON_ICON_GAP_WIDTH_PORT 12
#define EMOTICON_ICON_GAP_HEIGHT_PORT 6

#define EMOTICON_WIDTH_PORT (EMOTICON_ICON_WIDTH_PORT + EMOTICON_ICON_GAP_WIDTH_PORT)
#define EMOTICON_HEIGHT_PORT (EMOTICON_ICON_HEIGHT_PORT + EMOTICON_ICON_GAP_HEIGHT_PORT)

#define EMOTICON_ICON_WIDTH_LAND 30
#define EMOTICON_ICON_HEIGHT_LAND 30

#define EMOTICON_ICON_GAP_WIDTH_LAND 12
#define EMOTICON_ICON_GAP_HEIGHT_LAND 4

#define EMOTICON_WIDTH_LAND (EMOTICON_ICON_WIDTH_LAND + EMOTICON_ICON_GAP_WIDTH_LAND)
#define EMOTICON_HEIGHT_LAND (EMOTICON_ICON_HEIGHT_LAND + EMOTICON_ICON_GAP_HEIGHT_LAND)

#define MIN_RECENT_EMOTICON_NEEDED_IN_PORT 22        // (7*3 + 1)
#define MIN_RECENT_EMOTICON_NEEDED_IN_LAND 27        // (13*2 + 1)

#define MAX_SIZE_AMONG_FIRST_3_EMOTICON_GROUPS  (EMOTICON_GROUP_1_NUM > EMOTICON_GROUP_2_NUM ? (EMOTICON_GROUP_1_NUM > EMOTICON_GROUP_3_NUM ? EMOTICON_GROUP_1_NUM : EMOTICON_GROUP_3_NUM) : (EMOTICON_GROUP_2_NUM > EMOTICON_GROUP_3_NUM ? EMOTICON_GROUP_2_NUM : EMOTICON_GROUP_3_NUM))
#define MAX_SIZE_AMONG_EMOTICON_GROUPS (EMOTICON_GROUP_4_NUM > EMOTICON_GROUP_5_NUM ? (EMOTICON_GROUP_4_NUM > MAX_SIZE_AMONG_FIRST_3_EMOTICON_GROUPS ? EMOTICON_GROUP_4_NUM : MAX_SIZE_AMONG_FIRST_3_EMOTICON_GROUPS) : (EMOTICON_GROUP_5_NUM > MAX_SIZE_AMONG_FIRST_3_EMOTICON_GROUPS ? EMOTICON_GROUP_5_NUM : MAX_SIZE_AMONG_FIRST_3_EMOTICON_GROUPS))

bool is_emoticon_mode = false;
emoticon_group_t current_emoticon_group = EMOTICON_GROUP_RECENTLY_USED;
std::vector <int> emoticon_list_recent;
bool is_recently_used_emoticon_mode_disabled = true;

extern CSCLUI *g_ui;
extern CSCLCore g_core;
extern int * emoticon_list[];
extern CONFIG_VALUES g_config_values;

unsigned short int emoticon_group_items[MAX_EMOTICON_GROUP] =
{
    EMOTICON_GROUP_RECENTLY_USED_NUM,
    EMOTICON_GROUP_1_NUM,
    EMOTICON_GROUP_2_NUM,
    EMOTICON_GROUP_3_NUM,
    EMOTICON_GROUP_4_NUM,
    EMOTICON_GROUP_5_NUM
};

struct emoticon_item_t
{
    Elm_Object_Item *item;
    int code;
    std::string path;
    Eina_Unicode keyevent;
};

emoticon_item_t emoticon_items[MAX_SIZE_AMONG_EMOTICON_GROUPS] = {{0, }, };

// Static variable declarations
static Evas_Object *layout = NULL;
static Evas_Object *gengrid = NULL;
static Elm_Theme *theme = NULL;
static Elm_Gengrid_Item_Class *gic = NULL;

// Static Function declarations
static void __ise_emoticon_create_gengrid(unsigned short int screen_degree);
static void __ise_emoticon_append_items_to_gengrid(emoticon_group_t emoticon_group);
static void __ise_emoticon_create_item_class(unsigned short int screen_degree);
#if SUPPORTS_EMOTICONS_BY_IMAGE
static Evas_Object * grid_content_get(void *data, Evas_Object *obj, const char *part);
static void grid_content_del(void *data, Evas_Object *obj);
#else
static char * grid_text_get(void *data, Evas_Object *obj, const char *part);
#endif
static void _item_selected(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool _focus_done(void *data);
static void _multi_down(void *data, Evas *e, Evas_Object *o, void *event_info);
static void _multi_up(void *data, Evas *e, Evas_Object *o, void *event_info);

void ise_read_recent_emoticon_list_from_scim(void)
{
    LOGD("Enter\n");
    std::string string_value;
    g_core.config_read_string(ISE_CONFIG_RECENT_EMOTICONS_LIST, string_value);
    if (string_value.length() > 0) {
        LOGD("read recent emoticon:%s\n", string_value.c_str());
        std::stringstream ss(string_value);
        std::istream_iterator<std::string> begin(ss);
        std::istream_iterator<std::string> end;
        std::vector<std::string> vstrings(begin, end);

        emoticon_list_recent.clear();
        std::vector<std::string>::iterator it;
        for (it = vstrings.begin(); it != vstrings.end(); it++) {
            emoticon_list_recent.push_back(atoi((*it).c_str()));
        }
    }
}

void ise_write_recent_emoticon_list_to_scim(void)
{
    std::string string_value;
    for (std::vector<int>::iterator it = emoticon_list_recent.begin();
        it != emoticon_list_recent.end(); std::advance(it, 1)) {
            char buf[10]= {0};
            snprintf (buf, sizeof(buf), "%d", *it);
            string_value += std::string(buf);
            string_value += " ";
    }
    if (string_value.length() > 0) {
        g_core.config_write_string(ISE_CONFIG_RECENT_EMOTICONS_LIST, string_value);
        g_core.config_flush();
        LOGD("write recent emoticon:%s\n", string_value.c_str());
    }
}

void ise_show_emoticon_window(emoticon_group_t emoticon_group, const int screen_degree, const bool is_candidate_on, void *main_window)
{
#ifdef _TV
    return;
#endif
    //xt9_send_flush(0);
    ise_init_emoticon_list();
    ise_set_private_key_for_emoticon_mode(emoticon_group);

    if (emoticon_group == EMOTICON_GROUP_RECENTLY_USED) {
        //ise_read_recent_emoticon_list_from_scim();
        if (emoticon_list_recent.size() == 0) {
            //PRINTFUNC(DLOG_ERROR,"Cannot display recently used emoticons group. No recently used emoticons available");
            return;
        }
    }

    ise_destroy_emoticon_window();

    layout = elm_layout_add((Evas_Object *)main_window);
    evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);

    evas_object_move(layout, 0, 0);

    if (is_candidate_on) {
        if (screen_degree == 0 || screen_degree == 180)
            elm_layout_file_set(layout, EMOTICON_EDJ_FILE_PATH, EMOTICON_EDJ_GROUP_PORT_CANDIDATE_ON);
        else
            elm_layout_file_set(layout, EMOTICON_EDJ_FILE_PATH, EMOTICON_EDJ_GROUP_LAND_CANDIDATE_ON);
    } else {
        sclint width = 0;
        sclint height = 0;
        g_ui->get_screen_resolution(&width, &height);
        LOGD("screen width:%d, height:%d\n", width, height);
        if (screen_degree == 0 || screen_degree == 180) {
            elm_layout_file_set(layout, EMOTICON_EDJ_FILE_PATH, EMOTICON_EDJ_GROUP_PORT_CANDIDATE_OFF);
            evas_object_resize(layout, width, g_ui->get_scaled_y(ISE_HEIGHT_PORT));
        } else {
            elm_layout_file_set(layout, EMOTICON_EDJ_FILE_PATH, EMOTICON_EDJ_GROUP_LAND_CANDIDATE_OFF);
            evas_object_resize(layout, width, g_ui->get_scaled_y(ISE_HEIGHT_LAND));
        }
    }

    theme = elm_theme_new();
    elm_theme_ref_set(theme, NULL); // refer to default theme
    elm_theme_extension_add(theme, CUSTOM_GENGRID_EDJ_FILE_PATH);

    __ise_emoticon_create_gengrid((unsigned short)screen_degree);
    __ise_emoticon_create_item_class((unsigned short)screen_degree);
    __ise_emoticon_append_items_to_gengrid(emoticon_group);

    elm_object_part_content_set(layout, "emoticon.swallow.gengrid", gengrid);
    //elm_win_resize_object_add(main_window, layout);

    evas_object_show(gengrid);
    evas_object_layer_set(layout, EVAS_CANDIDATE_LAYER-1);
    evas_object_show(layout);

    is_emoticon_mode = true;
    current_emoticon_group = emoticon_group;
}

void ise_destroy_emoticon_window(void)
{
    /* According to UI FW team, when the a parent is deleted, all its child content will
     * also be deleted. So no need to delete gengrid explicitly. Just setting the gendrid
     * pointer to NULL should suffice here.
     * OR
     * We can first delete all the child elements and then we delete the parent.
     * This approach is currently used below
     */

    if (gengrid) {
        elm_gengrid_clear(gengrid);
        evas_object_del(gengrid);
        gengrid = NULL;
    }

    if (layout) {
        evas_object_del(layout);
        layout = NULL;
    }

    if (theme) {
        elm_theme_free(theme);
        theme = NULL;
    }
}

void ise_change_emoticon_mode(emoticon_group_t emoticon_group)
{
    if (emoticon_group == EMOTICON_GROUP_RECENTLY_USED) {
        //ise_read_recent_emoticon_list_from_scim();
        if (emoticon_list_recent.size() == 0) {
            //PRINTFUNC(DLOG_ERROR,"Cannot display recently used emoticons group. No recently used emoticons available");
            return;
        }
    }

    if (gengrid) {
        elm_gengrid_clear(gengrid);
    }

    __ise_emoticon_append_items_to_gengrid(emoticon_group);
    current_emoticon_group = emoticon_group;
}

static Eina_Bool _focus_done(void *data)
{
    Elm_Object_Item *item = (Elm_Object_Item *)data;

    elm_object_item_signal_emit(item, "mouse,up,1", "reorder_bg");

    return ECORE_CALLBACK_CANCEL;
}

static void _multi_down(void *data, Evas *e, Evas_Object *o, void *event_info)
{
    Evas_Event_Multi_Down *ev = (Evas_Event_Multi_Down*)event_info;

//  PRINTFUNC(DLOG_DEBUG,"MULTI: down @ %4i %4i | dev: %i\n", ev->canvas.x, ev->canvas.y, ev->device);
    if (ev->device >= IND_NUM)
        return;

    Elm_Object_Item *item;
    item = elm_gengrid_at_xy_item_get(o, ev->canvas.x, ev->canvas.y, NULL, NULL);

    elm_object_item_signal_emit(item, "mouse,down,1", "reorder_bg");

    ecore_timer_add(0.5, _focus_done, item);
}

static void _multi_up(void *data, Evas *e, Evas_Object *o, void *event_info)
{
    Evas_Event_Multi_Up *ev = (Evas_Event_Multi_Up*)event_info;
//  PRINTFUNC(DLOG_DEBUG,"MULTI: up    @ %4i %4i | dev: %i\n", ev->canvas.x, ev->canvas.y, ev->device);
    if (ev->device >= IND_NUM)
        return;

    Elm_Object_Item *item = NULL;
    item = elm_gengrid_at_xy_item_get(o, ev->canvas.x, ev->canvas.y, NULL, NULL);

    elm_object_item_signal_emit(item, "mouse,up,1", "reorder_bg");

    elm_gengrid_item_selected_set(item, EINA_TRUE);
}

static void __ise_emoticon_create_gengrid(unsigned short int screen_degree)
{
    gengrid = elm_gengrid_add(layout);
    elm_object_theme_set(gengrid, theme);
    //elm_object_style_set(gengrid,"no_effect");
    elm_gengrid_align_set(gengrid, 0.5, 0);

    evas_object_size_hint_weight_set(gengrid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(gengrid, EVAS_HINT_FILL, EVAS_HINT_FILL);

    if (screen_degree == 0 || screen_degree == 180)
        elm_gengrid_item_size_set(gengrid, ELM_SCALE_SIZE(EMOTICON_WIDTH_PORT), ELM_SCALE_SIZE(EMOTICON_HEIGHT_PORT));
    else
        elm_gengrid_item_size_set(gengrid, ELM_SCALE_SIZE(EMOTICON_WIDTH_LAND), ELM_SCALE_SIZE(EMOTICON_HEIGHT_LAND));

    elm_gengrid_highlight_mode_set(gengrid, EINA_TRUE);
    elm_gengrid_select_mode_set(gengrid, ELM_OBJECT_SELECT_MODE_ALWAYS);
//  elm_gengrid_cache_mode_set(gengrid, EINA_TRUE);
    elm_gengrid_multi_select_set(gengrid, EINA_TRUE);

    evas_object_event_callback_add(gengrid, EVAS_CALLBACK_MULTI_DOWN, _multi_down, NULL);
    evas_object_event_callback_add(gengrid, EVAS_CALLBACK_MULTI_UP, _multi_up, NULL);
}

#if SUPPORTS_EMOTICONS_BY_IMAGE
static void __ise_emoticon_append_items_to_gengrid(emoticon_group_t emoticon_group)
{
    char img_name[10];
    short int items = 0;
    std::string file_path = "";

    if (emoticon_group == EMOTICON_GROUP_RECENTLY_USED) {
        items = emoticon_list_recent.size();

        for (int i = 0; i < items; i++) {
            snprintf(img_name, 10, "%x", emoticon_list_recent[i]);
            emoticon_items[i].code = emoticon_list_recent[i];
            emoticon_items[i].keyevent = emoticon_list_recent[i];
            emoticon_items[i].path = (std::string)EMOTICON_DIR + (std::string)"u" + (std::string)img_name + (std::string)".png";
            emoticon_items[i].item = elm_gengrid_item_append(gengrid, gic, &(emoticon_items[i]), _item_selected, &(emoticon_items[i]));
        }
    } else {
        if (emoticon_group != EMOTICON_GROUP_DESTROY) {
            items = emoticon_group_items[emoticon_group];
        }
        for (int i = 0; i < items; i++) {
            snprintf(img_name, 10, "%x", emoticon_list[emoticon_group-1][i]);
            file_path = (std::string)EMOTICON_DIR + (std::string)"u" + (std::string)img_name + (std::string)".png";
            if (ise_util_does_file_exists(file_path)) {
                emoticon_items[i].code = emoticon_list[emoticon_group-1][i];
                emoticon_items[i].keyevent = emoticon_list[emoticon_group-1][i];
                emoticon_items[i].path = file_path;
                emoticon_items[i].item = elm_gengrid_item_append(gengrid, gic, &(emoticon_items[i]), _item_selected, &(emoticon_items[i]));
//              PRINTFUNC(SECURE_DEBUG,"file_path = %s\n",file_path.c_str());
            }
        }
    }
    Elm_Object_Item * it = elm_gengrid_first_item_get(gengrid);
    elm_gengrid_item_show(it, ELM_GENGRID_ITEM_SCROLLTO_NONE);
}


static void __ise_emoticon_create_item_class(unsigned short int screen_degree)
{
    if (!gic)
        gic = elm_gengrid_item_class_new();

    if (screen_degree == 0 || screen_degree == 180)
        gic->item_style = EMOTICON_GENGRID_ITEM_STYLE_PORT2;
    else
        gic->item_style = EMOTICON_GENGRID_ITEM_STYLE_LAND2;

    gic->func.text_get = NULL;
    gic->func.content_get = grid_content_get;
    gic->func.state_get = NULL;
    gic->func.del = grid_content_del;
}

static Evas_Object * grid_content_get(void *data, Evas_Object *obj, const char *part)
{
    emoticon_item_t *ti = (emoticon_item_t *)data;
    if (ti == NULL)
        return NULL;

    if (!strcmp(part, "elm.swallow.icon")) {
        if (ti->path.c_str()) {
            Eina_Bool is_image_file_set = false;
            Evas_Object *icon = elm_image_add(obj);
            is_image_file_set = elm_image_file_set(icon, ti->path.c_str(), NULL);
            if (!is_image_file_set) {
                //PRINTFUNC(SECURE_ERROR,"image \"%s\" not set",ti->path.c_str());
            } else {
                elm_image_aspect_fixed_set(icon, EINA_FALSE);
                elm_image_preload_disabled_set(icon, EINA_FALSE);
                evas_object_show(icon);
            }
            evas_object_data_set(icon, "code", (void *)ti->code);
            return icon;
        }
    }

    return NULL;
}

static void grid_content_del(void *data, Evas_Object *obj)
{
    emoticon_item_t *ti = (emoticon_item_t *)data;
    if (!ti)
        return;

    Evas_Object *icon = elm_object_item_part_content_get(ti->item, "elm.swallow.icon");
    if (icon) {
        evas_object_data_del(icon, "code");
    }
    return;
}
#else //SUPPORTS_EMOTICONS_BY_IMAGE
static void __ise_emoticon_append_items_to_gengrid(emoticon_group_t emoticon_group)
{
    char img_name[10];
    short int items = 0;
    std::string file_path = "";

    if (emoticon_group == EMOTICON_GROUP_RECENTLY_USED) {
        items = emoticon_list_recent.size();

        for (int i = 0; i < items; i++) {
            snprintf(img_name, 10, "%x", emoticon_list_recent[i]);
            emoticon_items[i].keyevent = emoticon_list_recent[i];
            emoticon_items[i].path = (std::string)img_name;
            emoticon_items[i].item = elm_gengrid_item_append(gengrid, gic, &(emoticon_items[i]), _item_selected, &(emoticon_items[i]));
        }
    } else {
        if (emoticon_group != EMOTICON_GROUP_DESTROY) {
            items = emoticon_group_items[emoticon_group];
        }
        for (int i = 0; i < items; i++) {
            snprintf(img_name, 10, "%x", emoticon_list[emoticon_group-1][i]);
            file_path = (std::string)img_name;
            emoticon_items[i].keyevent = emoticon_list[emoticon_group-1][i];
            emoticon_items[i].path = file_path;
            emoticon_items[i].item = elm_gengrid_item_append(gengrid, gic, &(emoticon_items[i]), _item_selected, &(emoticon_items[i]));
        }
    }
    Elm_Object_Item * it = elm_gengrid_first_item_get(gengrid);
    elm_gengrid_item_show(it, ELM_GENGRID_ITEM_SCROLLTO_NONE);
}

static void __ise_emoticon_create_item_class(unsigned short int screen_degree)
{
    if (!gic)
        gic = elm_gengrid_item_class_new();

    if (gic) {
        if (screen_degree == 0 || screen_degree == 180)
            gic->item_style = EMOTICON_GENGRID_ITEM_STYLE_PORT2;
        else
            gic->item_style = EMOTICON_GENGRID_ITEM_STYLE_LAND2;

        gic->func.text_get = grid_text_get;
        gic->func.content_get = NULL;
        gic->func.state_get = NULL;
        gic->func.del = NULL;
    }
}

static char * grid_text_get(void *data, Evas_Object *obj, const char *part)
{
    emoticon_item_t *ti = (emoticon_item_t *)data;

    char *utf_8 = NULL;
    int length = 0;

    if (!strcmp(part, "elm.text")) {
        if (ti && !(ti->path.empty())) {
            const Eina_Unicode unicode_event[2] = {ti->keyevent, 0};
            utf_8 = eina_unicode_unicode_to_utf8(unicode_event, &length);
            return (utf_8);
        }
    }

    return NULL;
}
#endif //SUPPORTS_EMOTICONS_BY_IMAGE

static void _item_selected(void *data, Evas_Object *obj, void *event_info)
{
    emoticon_item_t *ti = (emoticon_item_t *)data;
    char *utf_8 = NULL;
    int length = 0;

    std::vector<int>::iterator it;

    static CSCLUtils *utils = CSCLUtils::get_instance();
    if (utils) {
        if (g_config_values.sound_on)
            utils->play_sound(DEFAULT_SOUND_STYLE);
        if (g_config_values.vibration_on)
            utils->play_vibration(DEFAULT_VIBRATION_STYLE, DEFAULT_VIBRATION_DURATION);
    }

    if (event_info) {
        elm_gengrid_item_selected_set((Elm_Object_Item *)event_info, EINA_FALSE);
        if (ti && ti->keyevent) {
            const Eina_Unicode unicode_event[2] = {ti->keyevent, 0};
//          PRINTFUNC(DLOG_DEBUG,"unicode_event is %x",unicode_event);
            utf_8 = eina_unicode_unicode_to_utf8(unicode_event, &length);

            if (utf_8) {
                ise_send_string((sclchar *)utf_8);
                free(utf_8);
            }

            if (current_emoticon_group != EMOTICON_GROUP_RECENTLY_USED) {
                if (emoticon_list_recent.size() < EMOTICON_GROUP_RECENTLY_USED_NUM) {
                    it = find(emoticon_list_recent.begin(), emoticon_list_recent.end(), ti->keyevent);
                    if (it == emoticon_list_recent.end()) {    // Item not found
                        emoticon_list_recent.insert(emoticon_list_recent.begin(), ti->keyevent);
                    } else {
                        emoticon_list_recent.erase(it);
                        emoticon_list_recent.insert(emoticon_list_recent.begin(), ti->keyevent);
                    }
                }
                else {
                    it = find(emoticon_list_recent.begin(), emoticon_list_recent.end(), ti->keyevent);
                    if (it != emoticon_list_recent.end())
                        emoticon_list_recent.erase(it);
                    else
                        emoticon_list_recent.erase(emoticon_list_recent.end() - 1);

                    emoticon_list_recent.insert(emoticon_list_recent.begin(), ti->keyevent);
                }
                ise_write_recent_emoticon_list_to_scim();
                /*
                if (is_recently_used_emoticon_mode_disabled)
                    ise_disable_recently_used_emoticon_key(false);
                */
            }
        }
    }
    //PRINTFUNC(DLOG_DEBUG,"_item_selected() ends");
}

bool is_emoticon_show(void)
{
    return is_emoticon_mode;
}

void ise_set_private_key_for_emoticon_mode(const emoticon_group_t emoticon_group)
{
    if (g_ui) {
        const char *group_name = NULL;
        for (int id = EMOTICON_GROUP_RECENTLY_USED; id < MAX_EMOTICON_GROUP; id++) {
            group_name = ise_get_emoticon_group_name(id);
            if (group_name)
                g_ui->unset_private_key(group_name);
        }

        group_name = ise_get_emoticon_group_name(emoticon_group);
        if (group_name) {
            sclchar* imagebg[SCL_BUTTON_STATE_MAX] = {
               const_cast<sclchar*>("button/B09_Qwerty_btn_press.png"), const_cast<sclchar*>(""), const_cast<sclchar*>("")};

            g_ui->set_private_key(group_name, const_cast<sclchar*>(""), NULL, imagebg, 0, const_cast<sclchar*>(group_name), TRUE);
        }
    }
}
#endif
