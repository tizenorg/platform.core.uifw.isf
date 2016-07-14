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

#ifndef __CANDIDATE_MULTI_LINE_EFL_H__
#define __CANDIDATE_MULTI_LINE_EFL_H__
#include "candidate.h"
#include <Elementary.h>

#define CANDIDATE_BAR_NUM 8
#define CANDIDATE_MORE_ONE_LINE 8

typedef struct _candidate_item_size
{
    int width;
    int height;
} candidate_item_size;

class EflMultiLineCandidate: public Candidate
{
    public:
        EflMultiLineCandidate(Evas_Object *);
        ~EflMultiLineCandidate();
        void show();
        void hide();
        void update(const std::vector<std::string> &candidates);
        void rotate(int degree);
        int get_height();
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

        Evas_Object *get_candidate_item(candidate_item_size item_text_size);
        candidate_item_size get_candidate_item_text_size(std::string srcStr);
        Evas_Object * get_candidate_seperate_line_vertical();
        Evas_Object * get_candidate_seperate_line_horizon();

        std::vector<std::string> cur_candidates;
        Evas_Object *m_window;

        int m_degree;
        int m_screenWidth;
        int m_screenHeight;
        double m_screenRatio;
        std::string m_candidateFontName;
        int m_candidateFontSize;
        Evas_Object * m_stringWidthCalObj;
        Evas_Object * m_candidateScrollerBg;
        Evas_Object * m_candidateScroller;
        Evas_Object * m_candidateTable;
        Evas_Object * m_candidateMoreBtn;
        Evas_Object * m_candidateCloseBtn;
        Evas_Object * m_candidateMoreScrollerBg;
        Evas_Object * m_candidateMoreScroller;
        Evas_Object * m_candidateMoreTable;
};
#endif
