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

#include "candidate-efl.h"
#include <vector>
#include <string>
#include <algorithm>
#include <dlog.h>

using namespace std;

#define CANDIDATE_EDJ_FILE_PATH "/usr/share/isf/ise/ise-default/720x1280/default/sdk/edc/candidate-single.edj"
#include "configure.h"
static void
_mouse_down(void *data, Evas *e,
    Evas_Object *button, void *event_info)
{
    EflCandidate *candidate = (EflCandidate *)data;
    candidate->item_pressed(button);
}

static void
_mouse_up(void *data, Evas *e,
    Evas_Object *button, void *event_info)
{
    //EflCandidate *candidate = (EflCandidate *)data;
}

static void
_mouse_move(void *data, Evas *e,
    Evas_Object *button, void *event_info)
{
    //EflCandidate *candidate = (EflCandidate *)data;
}

void
EflCandidate::item_pressed(Evas_Object *item)
{
    const char *str = edje_object_part_text_get(
        item, "candidate");

    int index = -1;
    if (str) {
        vector<string>::const_iterator it =
            find(cur_candidates.begin(), cur_candidates.end(), str);
        if (it != cur_candidates.end()) {
            index = it-cur_candidates.begin();
        } else {
            index = -1;
        }
    }

    // notify listners the event happend
    EventDesc desc;
    desc.type = EventDesc::CANDIDATE_ITEM_MOUSE_DOWN;
    desc.text = str? str : "";
    desc.index = index;
    notify_listeners(desc);
}

void
EflCandidate::item_released(Evas_Object *item)
{
    const char *str = edje_object_part_text_get(
        item, "candidate");

    // notify listners the event happend
    EventDesc desc;
    desc.type = EventDesc::CANDIDATE_ITEM_MOUSE_UP;
    desc.text = str? str : "";
    notify_listeners(desc);
}

void
EflCandidate::item_moved(Evas_Object *item)
{
    const char *str = edje_object_part_text_get(
        item, "candidate");

    // notify listners the event happend
    EventDesc desc;
    desc.type = EventDesc::CANDIDATE_ITEM_MOUSE_MOVE;
    desc.text = str? str : "";
    notify_listeners(desc);
}

Evas_Object*
EflCandidate::create_item() {
    int ret;

    Evas_Object *item;
    item = edje_object_add(evas_object_evas_get(win));
    ret = edje_object_file_set(item, CANDIDATE_EDJ_FILE_PATH, "candidate_item");
    if (!ret) {
        LOGW("getting candidate item failed.");
        return NULL;
    }

    evas_object_event_callback_add(item, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, this);
    evas_object_event_callback_add(item, EVAS_CALLBACK_MOUSE_UP, _mouse_up, this);
    evas_object_event_callback_add(item, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move, this);

    evas_object_size_hint_min_set(item, item_config.width, item_config.height);
    edje_object_text_class_set(item, "candidate", item_config.text_font.c_str(), item_config.text_size);
    return item;
}

Evas_Object*
EflCandidate::create_seperate_line() {
    int ret;
    Evas_Object *seperate_line;

    seperate_line = edje_object_add (evas_object_evas_get (win));
    ret = edje_object_file_set (seperate_line, CANDIDATE_EDJ_FILE_PATH, "seperate_line");
    if (!ret) {
        LOGW("getting seperate line failed.");
        return NULL;
    }
    evas_object_show (seperate_line);
    evas_object_size_hint_min_set(seperate_line, 2, 80);
    return seperate_line;
}

EflCandidate::EflCandidate(Evas_Object *window)
{
    this->win = window;
    layout = elm_layout_add(window);

    int ret = elm_layout_file_set(layout, CANDIDATE_EDJ_FILE_PATH, "candidate");
    if (!ret) {
        throw "loading candidate layout file failed";
    }

    evas_object_resize(layout, candidate_config.width, candidate_config.height);
    evas_object_show(layout);

    scroller = elm_scroller_add(window);
    elm_scroller_bounce_set(scroller, EINA_TRUE, EINA_FALSE);
    elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
    evas_object_resize(scroller, candidate_config.width, candidate_config.height);
    evas_object_show(scroller);

    table = elm_table_add(window);
    elm_object_content_set(scroller, table);
    evas_object_show(table);

    elm_object_part_content_set(layout, "candidate.scroller", scroller);
    for (int i = 0; i < MAX_CANDIDATE; ++i) {
        candidates[i] = NULL;
        seperate_lines[i] = NULL;
    }
}

EflCandidate::~EflCandidate() {
}

void
EflCandidate::show() {
    evas_object_show(layout);
}

void
EflCandidate::hide() {
    evas_object_hide(layout);
}

void
EflCandidate::update(const vector<string> &vec_str) {
    cur_candidates = vec_str;

    /* first clear all the old candidate items */
    for (int i = 0; i < MAX_CANDIDATE; ++i) {
        if (candidates[i]) {
            evas_object_del(candidates[i]);
            candidates[i] = NULL;
        }
        if (seperate_lines[i]) {
            evas_object_del(seperate_lines[i]);
            seperate_lines[i] = NULL;
        }
    }

    /* second create new candidate items */
    int i = 0;
    vector<string>::const_iterator it;
    for (it = vec_str.begin(); it != vec_str.end();
            ++it) {
        candidates[i] = create_item();
        edje_object_part_text_set (candidates[i], "candidate", it->c_str());
        evas_object_show(candidates[i]);
        elm_table_pack(table, candidates[i], 2*i, 0, 1, 1);

        seperate_lines[i] = create_seperate_line();
        evas_object_show(seperate_lines[i]);
        elm_table_pack(table, seperate_lines[i], 2*i+1, 0, 1, 1);
        if (i == 0) {
            // set highlight of 0
            edje_object_color_class_set(candidates[0],
                "text_color",
                255, 0, 0, 255,
                255, 255, 255, 255,
                255, 255, 255, 255);
        }
        i++;
        if (i >= MAX_CANDIDATE) {
            break;
        }
    }

}
