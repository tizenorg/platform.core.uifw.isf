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

#ifdef SUPPORTS_EMOTICONS

#include "ise-emoticon-mode.h"
#include "ise.h"
#include <Elementary.h>
#include <algorithm>

#undef LOG_TAG
#define LOG_TAG "ISE_DEFAULT"
#include <dlog.h>

#define IND_NUM 20
#define EMOTICON_ICON_WIDTH_PORT 54
#define EMOTICON_ICON_HEIGHT_PORT 54

#define EMOTICON_ICON_GAP_WIDTH_PORT 18
#define EMOTICON_ICON_GAP_HEIGHT_PORT 6

#define EMOTICON_WIDTH_PORT (EMOTICON_ICON_WIDTH_PORT + EMOTICON_ICON_GAP_WIDTH_PORT)
#define EMOTICON_HEIGHT_PORT (EMOTICON_ICON_HEIGHT_PORT + EMOTICON_ICON_GAP_HEIGHT_PORT)

#define EMOTICON_ICON_WIDTH_LAND 72
#define EMOTICON_ICON_HEIGHT_LAND 72

#define EMOTICON_ICON_GAP_WIDTH_LAND 24
#define EMOTICON_ICON_GAP_HEIGHT_LAND 6

#define EMOTICON_WIDTH_LAND (EMOTICON_ICON_WIDTH_LAND + EMOTICON_ICON_GAP_WIDTH_LAND)
#define EMOTICON_HEIGHT_LAND (EMOTICON_ICON_HEIGHT_LAND + EMOTICON_ICON_GAP_HEIGHT_LAND)

#define MAX_SIZE_AMONG_FIRST_3_EMOTICON_GROUPS  (EMOTICON_GROUP_1_NUM > EMOTICON_GROUP_2_NUM ? (EMOTICON_GROUP_1_NUM > EMOTICON_GROUP_3_NUM ? EMOTICON_GROUP_1_NUM : EMOTICON_GROUP_3_NUM) : (EMOTICON_GROUP_2_NUM > EMOTICON_GROUP_3_NUM ? EMOTICON_GROUP_2_NUM : EMOTICON_GROUP_3_NUM))
#define MAX_SIZE_AMONG_EMOTICON_GROUPS (EMOTICON_GROUP_4_NUM > EMOTICON_GROUP_5_NUM ? (EMOTICON_GROUP_4_NUM > MAX_SIZE_AMONG_FIRST_3_EMOTICON_GROUPS ? EMOTICON_GROUP_4_NUM : MAX_SIZE_AMONG_FIRST_3_EMOTICON_GROUPS) : (EMOTICON_GROUP_5_NUM > MAX_SIZE_AMONG_FIRST_3_EMOTICON_GROUPS ? EMOTICON_GROUP_5_NUM : MAX_SIZE_AMONG_FIRST_3_EMOTICON_GROUPS))

#define EMOTION_SCREEN_WIDTH     540
#define EMOTION_SCREEN_HEIGHT    960
#define EMOTION_ITEM_LEFT_MARGIN 27
#define EMOTION_ITEM_TOP_MARGIN  9
#define EMOTION_SCROLLER_WIDTH   540
#define EMOTION_SCROLLER_HEIGHT  234

static double g_screen_width_rate = 0;
static double g_screen_height_rate = 0;

static bool is_emoticon_mode = false;
emoticon_group_t current_emoticon_group = EMOTICON_GROUP_RECENTLY_USED;
std::vector <int> emoticon_list_recent;
bool is_recently_used_emoticon_mode_disabled = true;
extern int * emoticon_list[];

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

emoticon_item_t emoticon_items[MAX_SIZE_AMONG_EMOTICON_GROUPS] = {{0,},};

static Evas_Object *g_main_window = NULL;
static Evas_Object *scroller = NULL;
static Evas_Object *layout = NULL;
static Evas_Object *gengrid = NULL;
static Elm_Theme *theme = NULL;
static Elm_Gengrid_Item_Class *gic = NULL;

// Function declarations
static void __ise_emoticon_create_gengrid(unsigned short int screen_degree);
static void __ise_emoticon_append_items_to_gengrid(emoticon_group_t emoticon_group);
static Evas_Object * grid_content_get(void *data, Evas_Object *obj, const char *part);
static void grid_content_del(void *data, Evas_Object *obj);
static void _item_selected(void *data, Evas_Object *obj, void *event_info);
static void __ise_emoticon_create_item_class(unsigned short int screen_degree);
static Eina_Bool _focus_done(void *data);
static void _multi_down(void *data, Evas *e, Evas_Object *o, void *event_info);
static void _multi_up(void *data, Evas *e, Evas_Object *o, void *event_info);

void ise_show_emoticon_window(emoticon_group_t emoticon_group, const int screen_degree, const bool is_candidate_on, void *main_window)
{
    g_main_window = (Evas_Object *)main_window;
    ise_destroy_emoticon_window();

    int w = 0;
    int h = 0;
    elm_win_screen_size_get(g_main_window, NULL, NULL, &w, &h);
    g_screen_width_rate = (double)w/EMOTION_SCREEN_WIDTH;
    g_screen_height_rate = (double)h/EMOTION_SCREEN_HEIGHT;
    scroller = elm_scroller_add((Evas_Object *)main_window);
    evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_TRUE);
    elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
    evas_object_move(scroller, g_screen_width_rate*EMOTION_ITEM_LEFT_MARGIN, g_screen_height_rate*EMOTION_ITEM_TOP_MARGIN);
    evas_object_resize(scroller, g_screen_width_rate*EMOTION_SCROLLER_WIDTH, g_screen_height_rate*EMOTION_SCROLLER_HEIGHT);

    __ise_emoticon_create_gengrid((unsigned short)screen_degree);
    __ise_emoticon_create_item_class((unsigned short)screen_degree);
    __ise_emoticon_append_items_to_gengrid(emoticon_group);

    elm_object_content_set(scroller, gengrid);

    evas_object_show(gengrid);
    evas_object_layer_set(gengrid, 32001);
    evas_object_layer_set(scroller, 32001);
    evas_object_show(scroller);

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

    if(gic) {
        elm_gengrid_item_class_free(gic);
        gic = NULL;
    }

    if(gengrid) {
        evas_object_hide(gengrid);
        elm_gengrid_clear(gengrid);
        evas_object_del(gengrid);
        gengrid = NULL;
    }

    if(scroller) {
        evas_object_hide(scroller);
        evas_object_del(scroller);
        scroller = NULL;
    }

    is_emoticon_mode = false;
}

void ise_change_emoticon_mode(emoticon_group_t emoticon_group)
{
    if(emoticon_group == EMOTICON_GROUP_RECENTLY_USED) {
//        ise_read_recent_emoticon_list_from_scim();
        if(emoticon_list_recent.size() == 0) {
//            PRINTFUNC(DLOG_ERROR,"Cannot display recently used emoticons group. No recently used emoticons available");
            return;
        }
    }

    if(gengrid) {
        elm_gengrid_clear(gengrid);
    }

    __ise_emoticon_append_items_to_gengrid(emoticon_group);
    current_emoticon_group = emoticon_group;
}

static Eina_Bool _focus_done(void *data)
{
    Elm_Object_Item *item = (Elm_Object_Item *)data;

    elm_object_item_signal_emit(item,"mouse,up,1","reorder_bg");

    return ECORE_CALLBACK_CANCEL;

}

static void _multi_down(void *data, Evas *e, Evas_Object *o, void *event_info)
{
    Evas_Event_Multi_Down *ev = (Evas_Event_Multi_Down*)event_info;

//    PRINTFUNC(DLOG_DEBUG,"MULTI: down @ %4i %4i | dev: %i\n", ev->canvas.x, ev->canvas.y, ev->device);
    if (ev->device >= IND_NUM)
        return;

    Elm_Object_Item *item;
    item = elm_gengrid_at_xy_item_get(o, ev->canvas.x, ev->canvas.y, NULL, NULL);

    elm_object_item_signal_emit(item,"mouse,down,1","reorder_bg");

    ecore_timer_add (0.5, _focus_done, item );
}

static void _multi_up(void *data, Evas *e, Evas_Object *o, void *event_info)
{
    Evas_Event_Multi_Up *ev = (Evas_Event_Multi_Up*)event_info;
//    PRINTFUNC(DLOG_DEBUG,"MULTI: up    @ %4i %4i | dev: %i\n", ev->canvas.x, ev->canvas.y, ev->device);
    if (ev->device >= IND_NUM)
        return;

    Elm_Object_Item *item = NULL;
    item = elm_gengrid_at_xy_item_get(o, ev->canvas.x, ev->canvas.y, NULL, NULL);

    elm_object_item_signal_emit(item,"mouse,up,1","reorder_bg");

    elm_gengrid_item_selected_set(item, EINA_TRUE);
}

static void __ise_emoticon_create_gengrid(unsigned short int screen_degree)
{
    gengrid = elm_gengrid_add(g_main_window);
    elm_object_style_set(gengrid,"no_effect");
    elm_gengrid_align_set(gengrid, 0, 0);
    evas_object_size_hint_weight_set(gengrid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(gengrid, EVAS_HINT_FILL, EVAS_HINT_FILL);
    if(screen_degree == 0 || screen_degree == 180){
        elm_gengrid_item_size_set(gengrid, EMOTICON_WIDTH_PORT*g_screen_width_rate, EMOTICON_HEIGHT_PORT*g_screen_height_rate);
    }
    else
        elm_gengrid_item_size_set(gengrid, EMOTICON_WIDTH_LAND, EMOTICON_HEIGHT_LAND);

    elm_gengrid_highlight_mode_set(gengrid, EINA_TRUE);
    elm_gengrid_select_mode_set(gengrid, ELM_OBJECT_SELECT_MODE_ALWAYS);
    elm_gengrid_multi_select_set(gengrid, EINA_FALSE);

//    evas_object_event_callback_add(gengrid, EVAS_CALLBACK_MULTI_DOWN, _multi_down, NULL);
//    evas_object_event_callback_add(gengrid, EVAS_CALLBACK_MULTI_UP, _multi_up, NULL);
}

static void __ise_emoticon_append_items_to_gengrid(emoticon_group_t emoticon_group)
{
    char img_name[10];
    short int items = 0;
    std::string file_path = "";

    if(emoticon_group == EMOTICON_GROUP_RECENTLY_USED) {
        items = emoticon_list_recent.size();

        for(int i = 0; i < items; i++) {
            emoticon_items[i].code = emoticon_list_recent[i];
            emoticon_items[i].keyevent = emoticon_list_recent[i];
            emoticon_items[i].item = elm_gengrid_item_append(gengrid, gic, &(emoticon_items[i]), _item_selected, &(emoticon_items[i]));
        }
    } else {
        if(emoticon_group != EMOTICON_GROUP_DESTROY) {
            items = emoticon_group_items[emoticon_group];
        }
        for (int i = 0; i < items; i++) {
            emoticon_items[i].code = emoticon_list[emoticon_group-1][i];
            emoticon_items[i].keyevent = emoticon_list[emoticon_group-1][i];
            emoticon_items[i].item = elm_gengrid_item_append(gengrid, gic, &(emoticon_items[i]), _item_selected, &(emoticon_items[i]));
        }
    }
//    Elm_Object_Item * it = elm_gengrid_first_item_get(gengrid);
//    elm_gengrid_item_show(it, ELM_GENGRID_ITEM_SCROLLTO_NONE);
}

static void __ise_emoticon_create_item_class(unsigned short int screen_degree)
{
    if(!gic)
        gic = elm_gengrid_item_class_new();

    gic->func.text_get = NULL;
    gic->func.content_get = grid_content_get;
    gic->func.state_get = NULL;
    gic->func.del = grid_content_del;
}

static Evas_Object * grid_content_get(void *data, Evas_Object *obj, const char *part)
{
    emoticon_item_t *ti = (emoticon_item_t *)data;
    if(ti == NULL)
        return NULL;

    if (!strcmp(part, "elm.swallow.icon")) {
        char chText[255] = {0};
        sprintf(chText, "&#x%x;", ti->code);
        Evas_Object *label = elm_label_add(obj);
        elm_object_text_set(label, chText);
        evas_object_resize(label, 72, 72);
        evas_object_show(label);
        evas_object_layer_set(label, 32001);
        return label;
    }

    return NULL;
}

static void grid_content_del(void *data, Evas_Object *obj)
{
    emoticon_item_t *ti = (emoticon_item_t *)data;
    if(!ti)
        return;

    Evas_Object *icon = elm_object_item_part_content_get(ti->item, "elm.swallow.icon");
    if(icon) {
        evas_object_hide(icon);
        evas_object_data_del(icon, "code");
    }
    return;
}

static void _item_selected(void *data, Evas_Object *obj, void *event_info)
{
    emoticon_item_t *ti = (emoticon_item_t *)data;
    char *utf_8 = NULL;
    int length = 0;

    std::vector<int>::iterator it;

    if(event_info) {
        elm_gengrid_item_selected_set((Elm_Object_Item *)event_info, EINA_FALSE);
        if(ti && ti->keyevent) {
            const Eina_Unicode unicode_event[2] = {ti->keyevent,0};
            utf_8 = eina_unicode_unicode_to_utf8(unicode_event, &length);
            if (utf_8) {
                ise_send_string((char *)utf_8);
                free(utf_8);
            }

            if(current_emoticon_group != EMOTICON_GROUP_RECENTLY_USED) {
                if(emoticon_list_recent.size() < EMOTICON_GROUP_RECENTLY_USED_NUM) {
                    it = find(emoticon_list_recent.begin(), emoticon_list_recent.end(), ti->keyevent);
                    if(it == emoticon_list_recent.end()) {    // Item not found
                        emoticon_list_recent.insert(emoticon_list_recent.begin(),ti->keyevent);
                    } else {
                        emoticon_list_recent.erase(it);
                        emoticon_list_recent.insert(emoticon_list_recent.begin(),ti->keyevent);
                    }
                }
                else {
                    it = find(emoticon_list_recent.begin(), emoticon_list_recent.end(), ti->keyevent);
                    emoticon_list_recent.erase(it);
                    emoticon_list_recent.insert(emoticon_list_recent.begin(),ti->keyevent);
                }
/*
                ise_write_recent_emoticon_list_to_scim();
                if(is_recently_used_emoticon_mode_disabled){
                    ise_disable_recently_used_emoticon_key(false);
                }
*/
            }
        }
    }
}

bool is_emoticon_show(void)
{
    return is_emoticon_mode;
}
#endif
