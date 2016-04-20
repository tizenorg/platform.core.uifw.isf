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

#include "candidate-multiline-efl.h"
#include <vector>
#include <string>
#include <assert.h>
#include <algorithm>

#define LOG_TAG "ISE_DEFAULT"
#include <dlog.h>

using namespace std;

#define CANDIDATE_EDJ_FILE_PATH       LAYOUTDIR"/sdk/edc/candidate-multiline.edj"
#define MORE_BUTTON_WIDTH             89
#define MORE_BUTTON_HEIGHT            64
#define CANDIDATE_WINDOW_HEIGHT       84
#define CANDIDATE_ITEMS_NUM           8
#define CANDIDATE_ITEM_BLANK          30
#define CANDIDATE_SEPERATE_WIDTH      2
#define CANDIDATE_SEPERATE_HEIGHT     52
#define CANDIDATE_HORIZON_LINE_HEIGHT 2

#ifdef _TV
#define IME_UI_RESOLUTION_W           1920
#define IME_UI_RESOLUTION_H           1080
#else
#define IME_UI_RESOLUTION_W           720
#define IME_UI_RESOLUTION_H           1280
#endif

static void
_mouse_down(void *data, Evas *e,
    Evas_Object *button, void *event_info)
{
    EflMultiLineCandidate *candidate = (EflMultiLineCandidate *)data;
    candidate->item_pressed(button);
}

static void
_mouse_up(void *data, Evas *e,
    Evas_Object *button, void *event_info)
{
    //EflMultiLineCandidate *candidate = (EflMultiLineCandidate *)data;
}

static void
_mouse_move(void *data, Evas *e,
    Evas_Object *button, void *event_info)
{
    //EflMultiLineCandidate *candidate = (EflMultiLineCandidate *)data;
}

void
EflMultiLineCandidate::item_pressed(Evas_Object *item)
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

    //hide_more_view();
    // notify listeners the event happened
    MultiEventDesc desc;
    desc.type = MultiEventDesc::CANDIDATE_ITEM_MOUSE_DOWN;
    desc.index = index;
    desc.text = str? str : "";
    notify_listeners(desc);
}

void
EflMultiLineCandidate::item_released(Evas_Object *item)
{
    // notify listners the event happend
    MultiEventDesc desc;
    desc.type = MultiEventDesc::CANDIDATE_ITEM_MOUSE_UP;
    notify_listeners(desc);
}

void
EflMultiLineCandidate::item_moved(Evas_Object *item)
{
    // notify listners the event happend
    MultiEventDesc desc;
    desc.type = MultiEventDesc::CANDIDATE_ITEM_MOUSE_MOVE;
    notify_listeners(desc);
}

Evas_Object*
EflMultiLineCandidate::get_candidate_item(candidate_item_size item_text_size)
{
    int ret;
    Evas_Object *edje;

    edje = edje_object_add(evas_object_evas_get(m_window));
    ret = edje_object_file_set(edje,
        CANDIDATE_EDJ_FILE_PATH, "candidate_item");
    if (!ret) {
        LOGW("getting candidate edje failed.\n");
        return NULL;
    }

    evas_object_event_callback_add(edje, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, this);
    evas_object_event_callback_add(edje, EVAS_CALLBACK_MOUSE_UP, _mouse_up, this);
    evas_object_event_callback_add(edje, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move, this);

    evas_object_size_hint_min_set(edje, item_text_size.width, CANDIDATE_WINDOW_HEIGHT*m_screenRatio);
    edje_object_text_class_set(edje, "candidate", m_candidateFontName.c_str(), m_candidateFontSize);
    evas_object_show(edje);

    return edje;
}

static void
ui_candidate_window_close_button_down_cb(void *data, Evas *e,
    Evas_Object *button, void *event_info)
{
    EflMultiLineCandidate *candidate =
        (EflMultiLineCandidate *)data;
    candidate->close_btn_clicked();
}

/*
static void
ui_candidate_window_more_button_up_cb(void *data, Evas *e,
    Evas_Object *button, void *event_info)
{
    EflMultiLineCandidate *candidate =
        (EflMultiLineCandidate *)data;
    candidate->more_btn_released();
}
*/

static void
ui_candidate_window_more_button_down_cb(void *data, Evas *e,
    Evas_Object *button, void *event_info)
{
    EflMultiLineCandidate *candidate =
        (EflMultiLineCandidate *)data;
    candidate->more_btn_clicked();
}

void
EflMultiLineCandidate::more_btn_released()
{
    MultiEventDesc desc;
    desc.type = MultiEventDesc::CANDIDATE_MORE_BTN_RELEASED;
    notify_listeners(desc);
}

void
EflMultiLineCandidate::hide_more_view()
{
    evas_object_hide(m_candidateMoreScrollerBg);
    evas_object_hide(m_candidateMoreScroller);
    evas_object_hide(m_candidateMoreTable);

    MultiEventDesc desc;
    desc.type = MultiEventDesc::CANDIDATE_MORE_VIEW_HIDE;

    notify_listeners(desc);
}

void
EflMultiLineCandidate::show_more_view()
{
    evas_object_show(m_candidateMoreScrollerBg);
    evas_object_show(m_candidateMoreScroller);
    evas_object_show(m_candidateMoreTable);

    MultiEventDesc desc;
    desc.type = MultiEventDesc::CANDIDATE_MORE_VIEW_SHOW;

    notify_listeners(desc);
}

void
EflMultiLineCandidate::show_view()
{
    evas_object_show(m_candidateScrollerBg);
    evas_object_show(m_candidateScroller);
    evas_object_show(m_candidateTable);
}

void
EflMultiLineCandidate::hide_view()
{
    evas_object_hide(m_candidateMoreBtn);
    evas_object_hide(m_candidateCloseBtn);
    evas_object_hide(m_candidateScrollerBg);
    evas_object_hide(m_candidateTable);
    evas_object_hide(m_candidateScroller);
}

void
EflMultiLineCandidate::more_btn_clicked()
{
    evas_object_hide(m_candidateMoreBtn);
    evas_object_show(m_candidateCloseBtn);
    show_more_view();

    MultiEventDesc desc;
    desc.type = MultiEventDesc::CANDIDATE_MORE_BTN_CLICKED;
    notify_listeners(desc);
}

void
EflMultiLineCandidate::close_btn_clicked()
{
    evas_object_hide(m_candidateCloseBtn);
    evas_object_show(m_candidateMoreBtn);
    hide_more_view();

    // notify listeners
    MultiEventDesc desc;
    desc.type = MultiEventDesc::CANDIDATE_CLOSE_BTN_CLICKED;
    notify_listeners(desc);
}

Evas_Object *
EflMultiLineCandidate::get_candidate_seperate_line_vertical()
{
    int ret = -1;

    Evas_Object *edje = NULL;

    edje = edje_object_add(evas_object_evas_get(m_window));
    ret = edje_object_file_set(edje, CANDIDATE_EDJ_FILE_PATH, "seperate_line_vertical");
    if (!ret) {
        LOGW("getting seperate line failed.\n");
        return NULL;
    }
    evas_object_show(edje);
    evas_object_size_hint_min_set(edje, CANDIDATE_SEPERATE_WIDTH, CANDIDATE_SEPERATE_HEIGHT*m_screenRatio);

    return edje;
}

Evas_Object *
EflMultiLineCandidate::get_candidate_seperate_line_horizon()
{
    int ret;
    Evas_Object * edje = NULL;
    edje = edje_object_add(evas_object_evas_get(m_window));
    ret = edje_object_file_set(edje, CANDIDATE_EDJ_FILE_PATH, "seperate_line_horizon");
    if (!ret) {
        LOGW("getting seperate line failed.\n");
        return NULL;
    }
    evas_object_show(edje);
    evas_object_size_hint_min_set(edje, m_screenWidth, CANDIDATE_HORIZON_LINE_HEIGHT);
    return edje;
}

void
EflMultiLineCandidate::make_more_view()
{
    m_candidateMoreScrollerBg = edje_object_add(evas_object_evas_get((Evas_Object*)m_window));
    edje_object_file_set(m_candidateMoreScrollerBg, CANDIDATE_EDJ_FILE_PATH, "scroller_bg");
    evas_object_size_hint_weight_set(m_candidateMoreScrollerBg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_resize(m_candidateMoreScrollerBg, m_screenWidth, m_screenHeight-CANDIDATE_WINDOW_HEIGHT*m_screenRatio);
    evas_object_move(m_candidateMoreScrollerBg, 0, CANDIDATE_WINDOW_HEIGHT*m_screenRatio);

    m_candidateMoreScroller = elm_scroller_add((Evas_Object*)m_window);
    elm_scroller_bounce_set(m_candidateMoreScroller, EINA_FALSE, EINA_TRUE);
    elm_scroller_policy_set(m_candidateMoreScroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
    evas_object_resize(m_candidateMoreScroller, m_screenWidth, m_screenHeight-CANDIDATE_WINDOW_HEIGHT*m_screenRatio);
    evas_object_move(m_candidateMoreScroller, 0, CANDIDATE_WINDOW_HEIGHT*m_screenRatio);

    m_candidateMoreTable = elm_table_add((Evas_Object*)m_window);
    evas_object_size_hint_weight_set(m_candidateMoreTable, 0.0, 0.0);
    evas_object_size_hint_align_set(m_candidateMoreTable, 0.0, 0.0);
    elm_table_padding_set(m_candidateMoreTable, 0, 0);
    elm_object_content_set(m_candidateMoreScroller, m_candidateMoreTable);

    evas_object_layer_set(m_candidateMoreScrollerBg, 32000);
    evas_object_layer_set(m_candidateMoreScroller, 32000);
    evas_object_layer_set(m_candidateMoreTable, 32000);
}

void
EflMultiLineCandidate::make_view()
{
    Evas_Coord scr_w, scr_h;
    elm_win_screen_size_get(m_window, NULL, NULL, &scr_w, &scr_h);
    m_screenWidth = scr_w;
    m_screenHeight = scr_h;
    double screenRatio_w = (double)scr_w/IME_UI_RESOLUTION_W;
    double screenRatio_h = (double)scr_h/IME_UI_RESOLUTION_H;
    m_screenRatio = MAX(screenRatio_w, screenRatio_h);

    m_candidateScrollerBg = edje_object_add(evas_object_evas_get((Evas_Object*)m_window));
    edje_object_file_set(m_candidateScrollerBg, CANDIDATE_EDJ_FILE_PATH, "candidate_bg");
    evas_object_size_hint_weight_set(m_candidateScrollerBg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_resize(m_candidateScrollerBg, scr_w, CANDIDATE_WINDOW_HEIGHT*m_screenRatio);
    evas_object_move(m_candidateScrollerBg, 0, 0);

    m_candidateScroller = elm_scroller_add(m_window);
    elm_scroller_bounce_set(m_candidateScroller, EINA_TRUE, EINA_FALSE);
    elm_scroller_policy_set(m_candidateScroller, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
    evas_object_resize(m_candidateScroller, scr_w, CANDIDATE_WINDOW_HEIGHT*m_screenRatio);
    evas_object_move(m_candidateScroller, 0, 0);

    m_candidateTable = elm_table_add(m_window);
    evas_object_size_hint_weight_set(m_candidateTable, 0.0, 0.0);
    evas_object_size_hint_align_set(m_candidateTable, 0.0, 0.0);
    elm_table_padding_set(m_candidateTable, 0, 0);
    elm_object_content_set(m_candidateScroller, m_candidateTable);

    m_candidateMoreBtn = edje_object_add(evas_object_evas_get(m_window));
    edje_object_file_set(m_candidateMoreBtn, CANDIDATE_EDJ_FILE_PATH, "more_button");
    evas_object_size_hint_min_set(m_candidateMoreBtn, MORE_BUTTON_WIDTH*m_screenRatio, MORE_BUTTON_HEIGHT*m_screenRatio);
    evas_object_resize(m_candidateMoreBtn, MORE_BUTTON_WIDTH*m_screenRatio, MORE_BUTTON_HEIGHT*m_screenRatio);
    evas_object_move(m_candidateMoreBtn, m_screenWidth-MORE_BUTTON_WIDTH*m_screenRatio,
                     (CANDIDATE_WINDOW_HEIGHT-MORE_BUTTON_HEIGHT)*m_screenRatio/2);
    evas_object_event_callback_add(m_candidateMoreBtn, EVAS_CALLBACK_MOUSE_DOWN,
                                   ui_candidate_window_more_button_down_cb, this);

    m_candidateCloseBtn = edje_object_add(evas_object_evas_get(m_window));
    edje_object_file_set(m_candidateCloseBtn, CANDIDATE_EDJ_FILE_PATH, "close_button");
    evas_object_size_hint_min_set(m_candidateCloseBtn, MORE_BUTTON_WIDTH*m_screenRatio, MORE_BUTTON_HEIGHT*m_screenRatio);
    evas_object_resize(m_candidateCloseBtn, MORE_BUTTON_WIDTH*m_screenRatio, MORE_BUTTON_HEIGHT*m_screenRatio);
    evas_object_move(m_candidateCloseBtn, m_screenWidth-MORE_BUTTON_WIDTH*m_screenRatio,
                     (CANDIDATE_WINDOW_HEIGHT-MORE_BUTTON_HEIGHT)*m_screenRatio/2);
    evas_object_event_callback_add(m_candidateCloseBtn, EVAS_CALLBACK_MOUSE_DOWN,
                                   ui_candidate_window_close_button_down_cb, this);

    evas_object_layer_set(m_candidateScrollerBg, 32000);
    evas_object_layer_set(m_candidateScroller, 32000);
    evas_object_layer_set(m_candidateMoreBtn, 32000);
    evas_object_layer_set(m_candidateCloseBtn, 32000);
}

EflMultiLineCandidate::EflMultiLineCandidate(Evas_Object *window)
{
    m_degree = 0;
    m_screenWidth = 0;
    m_screenHeight = 0;
    m_screenRatio = 1.0;
    m_window = window;
    make_view();
    make_more_view();
    m_candidateFontName = string("Tizen");
    m_candidateFontSize = 37*m_screenRatio;
    m_stringWidthCalObj = evas_object_text_add(m_window);
    evas_object_text_font_set(m_stringWidthCalObj, m_candidateFontName.c_str(), m_candidateFontSize);
}

EflMultiLineCandidate::~EflMultiLineCandidate()
{
    elm_table_clear(m_candidateTable, EINA_TRUE);
    elm_table_clear(m_candidateMoreTable, EINA_TRUE);

    evas_object_del(m_stringWidthCalObj);
    evas_object_del(m_candidateScrollerBg);
    evas_object_del(m_candidateScroller);
    evas_object_del(m_candidateTable);
    evas_object_del(m_candidateMoreBtn);
    evas_object_del(m_candidateCloseBtn);
    evas_object_del(m_candidateMoreScrollerBg);
    evas_object_del(m_candidateMoreScroller);
    evas_object_del(m_candidateMoreTable);
}

void
EflMultiLineCandidate::show()
{
    show_view();
}

void
EflMultiLineCandidate::hide()
{
    hide_view();
    hide_more_view();
}

candidate_item_size
EflMultiLineCandidate::get_candidate_item_text_size(string srcStr)
{
    evas_object_text_font_set(m_stringWidthCalObj, m_candidateFontName.c_str(), m_candidateFontSize);
    evas_object_text_text_set(m_stringWidthCalObj, srcStr.c_str());
    candidate_item_size item_text_size = {0, 0};
    evas_object_geometry_get(m_stringWidthCalObj, NULL, NULL, &item_text_size.width, &item_text_size.height);
//    int candidate_item_min = (m_screenWidth - MORE_BUTTON_WIDTH)/CANDIDATE_ITEMS_NUM;
    item_text_size.width += CANDIDATE_ITEM_BLANK * m_screenRatio * 2;
    item_text_size.width = MIN(item_text_size.width, m_screenWidth-MORE_BUTTON_WIDTH*m_screenRatio);
    return item_text_size;
}

void
EflMultiLineCandidate::update(const vector<string> &vec_str)
{
    cur_candidates = vec_str;
    hide_more_view();
    evas_object_hide(m_candidateMoreBtn);
    evas_object_hide(m_candidateCloseBtn);

    elm_table_clear(m_candidateTable, EINA_TRUE);
    elm_table_clear(m_candidateMoreTable, EINA_TRUE);

    int cur_item_sum = 0;
    int item_num = vec_str.size();
    bool multiline = false;
    int lineCount = 0;
    for (int i = 0; i < item_num; ++i)
    {
        candidate_item_size item_text_size = get_candidate_item_text_size(vec_str[i]);
        if ((!multiline) && (cur_item_sum + item_text_size.width < m_screenWidth - MORE_BUTTON_WIDTH*m_screenRatio))
        {
            int y_start = 0;
            if (0 != cur_item_sum)
            {
                Evas_Object *vertical_seperate = get_candidate_seperate_line_vertical();
                y_start = (CANDIDATE_WINDOW_HEIGHT - CANDIDATE_SEPERATE_HEIGHT)*m_screenRatio / 2;
                elm_table_pack(m_candidateTable, vertical_seperate, cur_item_sum, y_start,
                               CANDIDATE_SEPERATE_WIDTH, CANDIDATE_SEPERATE_HEIGHT*m_screenRatio);
                cur_item_sum += CANDIDATE_SEPERATE_WIDTH;
            }
            Evas_Object *item = get_candidate_item(item_text_size);
            edje_object_part_text_set(item, "candidate", vec_str[i].c_str());
            y_start = (CANDIDATE_WINDOW_HEIGHT*m_screenRatio - item_text_size.height)/2;
            elm_table_pack(m_candidateTable, item, cur_item_sum, y_start,
                           item_text_size.width, item_text_size.height);
            cur_item_sum += item_text_size.width;
        }
        else if (!multiline)
        {
            multiline = true;
            cur_item_sum = 0;
            if (0 == i)
            {
                int y_start = 0;
                Evas_Object *item = get_candidate_item(item_text_size);
                edje_object_part_text_set(item, "candidate", vec_str[i].c_str());
                y_start = (CANDIDATE_WINDOW_HEIGHT*m_screenRatio - item_text_size.height)/2;
                elm_table_pack(m_candidateTable, item, cur_item_sum, y_start,
                               item_text_size.width, item_text_size.height);
                continue;
            }
        }

        if (multiline)
        {
            if (cur_item_sum + item_text_size.width > m_screenWidth)
            {
                cur_item_sum = 0;
                lineCount++;
            }

            int y_start = 0;
            if (0 != cur_item_sum)
            {
                Evas_Object *vertical_seperate = get_candidate_seperate_line_vertical();
                y_start = (CANDIDATE_WINDOW_HEIGHT - CANDIDATE_SEPERATE_HEIGHT)*m_screenRatio / 2;
                elm_table_pack(m_candidateMoreTable, vertical_seperate, cur_item_sum,
                        lineCount*CANDIDATE_WINDOW_HEIGHT*m_screenRatio+(lineCount+1)*CANDIDATE_HORIZON_LINE_HEIGHT+y_start,
                        CANDIDATE_SEPERATE_WIDTH, CANDIDATE_SEPERATE_HEIGHT*m_screenRatio);
                cur_item_sum += CANDIDATE_SEPERATE_WIDTH;
            }
            else
            {
                Evas_Object *horizon_seperate = get_candidate_seperate_line_horizon();
                y_start = lineCount*(CANDIDATE_WINDOW_HEIGHT*m_screenRatio+CANDIDATE_HORIZON_LINE_HEIGHT);
                elm_table_pack(m_candidateMoreTable, horizon_seperate, cur_item_sum, y_start,
                        m_screenWidth, CANDIDATE_HORIZON_LINE_HEIGHT);
            }

            Evas_Object *item = get_candidate_item(item_text_size);
            edje_object_part_text_set(item, "candidate", vec_str[i].c_str());
            y_start = (CANDIDATE_WINDOW_HEIGHT*m_screenRatio - item_text_size.height)/2;
            elm_table_pack(m_candidateMoreTable, item, cur_item_sum, lineCount*CANDIDATE_WINDOW_HEIGHT*m_screenRatio+y_start,
                           item_text_size.width, item_text_size.height);
            cur_item_sum += item_text_size.width;
        }
    }

    if (multiline)
    {
        evas_object_show(m_candidateMoreBtn);
    }
}

void
EflMultiLineCandidate::rotate(int degree)
{
    m_degree = degree;

    Evas_Coord scr_w, scr_h;
    elm_win_screen_size_get(m_window, NULL, NULL, &scr_w, &scr_h);
    m_screenWidth = scr_w;
    m_screenHeight = scr_h;

    switch (degree) {
        case 0:
            m_screenWidth = MIN(scr_w, scr_h);
            m_screenHeight = MAX(scr_w, scr_h);
            break;
        case 180:
            m_screenWidth = MIN(scr_w, scr_h);
            m_screenHeight = MAX(scr_w, scr_h);
            break;
        case 90:
            m_screenWidth = MAX(scr_w, scr_h);
            m_screenHeight = MIN(scr_w, scr_h);
            break;
        case 270:
            m_screenWidth = MAX(scr_w, scr_h);
            m_screenHeight = MIN(scr_w, scr_h);
            break;
        default:
            break;
    }

#ifdef _TV
    int temp = m_screenWidth;
    m_screenWidth = m_screenHeight;
    m_screenHeight = temp;
#endif

    evas_object_resize(m_candidateScrollerBg, m_screenWidth, CANDIDATE_WINDOW_HEIGHT*m_screenRatio);
    evas_object_resize(m_candidateScroller, m_screenWidth, CANDIDATE_WINDOW_HEIGHT*m_screenRatio);
    evas_object_resize(m_candidateMoreScrollerBg, m_screenWidth, m_screenHeight-CANDIDATE_WINDOW_HEIGHT*m_screenRatio);
    evas_object_resize(m_candidateMoreScroller, m_screenWidth, m_screenHeight-CANDIDATE_WINDOW_HEIGHT*m_screenRatio);
    evas_object_move(m_candidateMoreBtn, m_screenWidth-MORE_BUTTON_WIDTH*m_screenRatio,
                     (CANDIDATE_WINDOW_HEIGHT-MORE_BUTTON_HEIGHT)*m_screenRatio/2);
    evas_object_move(m_candidateCloseBtn, m_screenWidth-MORE_BUTTON_WIDTH*m_screenRatio,
                     (CANDIDATE_WINDOW_HEIGHT-MORE_BUTTON_HEIGHT)*m_screenRatio/2);
}
