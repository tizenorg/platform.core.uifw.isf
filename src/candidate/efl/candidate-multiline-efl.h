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

#ifndef __CANDIDATE_MULTI_LINE_EFL_H__
#define __CANDIDATE_MULTI_LINE_EFL_H__
#include "candidate.h"
#include <Elementary.h>

#define CANDIDATE_BAR_NUM 8
#define CANDIDATE_MORE_ONE_LINE 8

class EflMultiLineCandidate: public Candidate
{
    public:
        EflMultiLineCandidate(Evas_Object *);
        ~EflMultiLineCandidate();
        void show();
        void hide();
        void update(const std::vector<std::string> &candidates);
        void rotate(int degree);
        void more_btn_clicked();
        void more_btn_released();
        void close_btn_clicked();
        void item_pressed(Evas_Object *);
        void item_released(Evas_Object *);
        void item_moved(Evas_Object *);
    private:
        void make_view();
        void make_more_view();
        void show_more_view();
        void hide_more_view();
        void show_view();
        void hide_view();

        Evas_Object *get_candidate_close_button();
        Evas_Object *get_candidate_more_button();
        Evas_Object *get_candidate_item();
        std::vector<std::string> cur_candidates;

        Evas_Object *win;
        // used for normal candidate
        struct View {
            Evas_Object *layout;
            Evas_Object *scroller;
            Evas_Object *table;
        }view;
        // used when more button clicked
        struct More_View {
            Evas_Object *layout;
            Evas_Object *more_scroller;
            Evas_Object *more_table;
        }more_view;

        int degree;
};
#endif
