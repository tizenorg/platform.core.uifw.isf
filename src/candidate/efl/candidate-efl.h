#ifndef __CANDIDATE_EFL_H__
#define __CANDIDATE_EFL_H__
#include "candidate.h"

#include <Elementary.h>
#include <vector>
#include <string>

#define MAX_CANDIDATE 40

class EflCandidate: public Candidate
{
    public:
        EflCandidate(Evas_Object *);
        ~EflCandidate();
        void show();
        void hide();
        void update(const std::vector<std::string> &candidates);
        void item_pressed(Evas_Object *item);
        void item_released(Evas_Object *item);
        void item_moved(Evas_Object *item);
    private:
        Evas_Object *create_item();
        Evas_Object *create_seperate_line();
        Evas_Object *win;
        Evas_Object *layout;
        Evas_Object *background;
        Evas_Object *scroller;
        Evas_Object *table;
        Evas_Object *candidates[MAX_CANDIDATE];
        Evas_Object *seperate_lines[MAX_CANDIDATE];

        std::vector<std::string> cur_candidates;
};
#endif
