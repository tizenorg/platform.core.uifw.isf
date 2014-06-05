#include "candidate-multiline-efl.h"
#include <vector>
#include <string>
#include <assert.h>
#include <algorithm>
using namespace std;

#define CANDIDATE_EDJ_FILE_PATH "/usr/share/isf/ise/ise-default/720x1280/default/sdk/edc/candidate-multiline.edj"
static Evas_Object *_get_seperate_line(Evas_Object *win);

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

    int index;
    vector<string>::const_iterator it =
        find(cur_candidates.begin(), cur_candidates.end(), str);
    if (it != cur_candidates.end()) {
        index = it-cur_candidates.begin();
    } else {
        index = -1;
    }
    if (it != cur_candidates.end()) {
        index = it-cur_candidates.begin();
    } else {
        index = -1;
    }

    //hide_more_view();
    // notify listners the event happend
    MultiEventDesc desc;
    desc.type = MultiEventDesc::CANDIDATE_ITEM_MOUSE_DOWN;
    desc.index = index;
    desc.text = str;
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
EflMultiLineCandidate::get_candidate_item()
{
    int ret;
    Evas_Object *edje;

    edje = edje_object_add(evas_object_evas_get(win));
    ret = edje_object_file_set(edje,
        CANDIDATE_EDJ_FILE_PATH, "candidate_item");
    if (!ret) {
        printf("getting candidate edje failed.\n");
        return NULL;
    }

    evas_object_event_callback_add(edje, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, this);
    evas_object_event_callback_add(edje, EVAS_CALLBACK_MOUSE_UP, _mouse_up, this);
    evas_object_event_callback_add(edje, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move, this);

    evas_object_size_hint_min_set(edje, 89, 84);
    evas_object_size_hint_weight_set(edje, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(edje, EVAS_HINT_FILL, EVAS_HINT_FILL);
    edje_object_text_class_set(edje, "candidate", "arial", 22);
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

static void
ui_candidate_window_more_button_up_cb(void *data, Evas *e,
    Evas_Object *button, void *event_info)
{
    EflMultiLineCandidate *candidate =
        (EflMultiLineCandidate *)data;
    candidate->more_btn_released();
}

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
    evas_object_hide(more_view.layout);

    MultiEventDesc desc;
    desc.type = MultiEventDesc::CANDIDATE_MORE_VIEW_HIDE;

    notify_listeners(desc);
}

void
EflMultiLineCandidate::show_more_view()
{
    evas_object_show(more_view.layout);
    evas_object_layer_set(more_view.layout, 32000);

    MultiEventDesc desc;
    desc.type = MultiEventDesc::CANDIDATE_MORE_VIEW_SHOW;

    notify_listeners(desc);
}

void
EflMultiLineCandidate::show_view()
{
    evas_object_show(view.layout);
}

void
EflMultiLineCandidate::hide_view()
{
    evas_object_hide(view.layout);
}

void
EflMultiLineCandidate::more_btn_clicked()
{
    show_more_view();

    elm_table_clear(view.table, EINA_TRUE);
    //assert(cur_candidates.size() > CANDIDATE_BAR_NUM);
    for (size_t i = 0; i < CANDIDATE_BAR_NUM-1; ++i) {
        Evas_Object *item = get_candidate_item();
        edje_object_part_text_set (item, "candidate", cur_candidates.at(i).c_str());
        elm_table_pack(view.table, item, 2*i, 0, 1, 1);
        Evas_Object *seperate = _get_seperate_line(win);
        elm_table_pack(view.table, seperate, 2*i+1, 0, 1, 1);
    }
    // create candidate more/close button
    Evas_Object *btn = get_candidate_close_button();
    elm_table_pack(view.table, btn,
            2*(CANDIDATE_BAR_NUM-1), 0, 1, 1);

    // update more view
    elm_table_clear(more_view.more_table, EINA_TRUE);
    // fill in the more items
    int i = 0;
    int j = 0;
    for (size_t n = CANDIDATE_BAR_NUM-1; n < cur_candidates.size(); ++n) {
        Evas_Object *item = get_candidate_item();
        edje_object_part_text_set (item, "candidate", cur_candidates[n].c_str());
        elm_table_pack(more_view.more_table, item, 2*j, i, 1, 1);
        Evas_Object *seperate = _get_seperate_line(win);
        elm_table_pack(more_view.more_table, seperate, 2*j+1, i, 1, 1);
        j++;
        if (j >= CANDIDATE_MORE_ONE_LINE) {
            j = 0;
            i++;
        }
    }
    MultiEventDesc desc;
    desc.type = MultiEventDesc::CANDIDATE_MORE_BTN_CLICKED;
    notify_listeners(desc);
}

void
EflMultiLineCandidate::close_btn_clicked()
{
    hide_more_view();

    update(cur_candidates);
    // notify listeners
    MultiEventDesc desc;
    desc.type = MultiEventDesc::CANDIDATE_CLOSE_BTN_CLICKED;
    notify_listeners(desc);
}

Evas_Object *
EflMultiLineCandidate::get_candidate_more_button ()
{
    Evas_Object *edje;

    edje = edje_object_add (evas_object_evas_get (win));
    edje_object_file_set (edje,
        CANDIDATE_EDJ_FILE_PATH, "more_button");
    evas_object_size_hint_min_set(edje, 100, 84);
    evas_object_show(edje);
    evas_object_event_callback_add (edje, EVAS_CALLBACK_MOUSE_DOWN, ui_candidate_window_more_button_down_cb, this);

    return edje;
}

Evas_Object *
EflMultiLineCandidate::get_candidate_close_button ()
{
    Evas_Object *edje;

    edje = edje_object_add (evas_object_evas_get (win));
    edje_object_file_set (edje,
        CANDIDATE_EDJ_FILE_PATH, "close_button");
    evas_object_size_hint_min_set(edje, 100, 84);
    evas_object_show(edje);
    evas_object_event_callback_add (edje, EVAS_CALLBACK_MOUSE_DOWN, ui_candidate_window_close_button_down_cb, this);
    return edje;
}

static Evas_Object *
_get_seperate_line(Evas_Object *win) {
    int ret;

    Evas_Object *edje;

    edje = edje_object_add (evas_object_evas_get (win));
    ret = edje_object_file_set (edje, CANDIDATE_EDJ_FILE_PATH, "seperate_line");
    if (!ret) {
        printf("getting seperate line failed.\n");
        return NULL;
    }
    evas_object_show (edje);
    evas_object_size_hint_min_set(edje, 1, 80);
    return edje;
}

void
EflMultiLineCandidate::make_more_view()
{
    more_view.layout = edje_object_add(evas_object_evas_get(win));
    int ret = edje_object_file_set(more_view.layout,
        CANDIDATE_EDJ_FILE_PATH, "candidate_more_view");
    if (!ret) {
        printf("error while loading more candidate layout\n");
        throw "failed loading candidate layout.";
    }

    evas_object_resize(more_view.layout, 720, 444);
    evas_object_move(more_view.layout, 0, 86);
    evas_object_show(more_view.layout);

    // create candidate more part
    more_view.more_scroller = elm_scroller_add(win);
    elm_scroller_bounce_set (
        more_view.more_scroller, EINA_TRUE, EINA_FALSE);
    elm_scroller_policy_set (
        more_view.more_scroller, ELM_SCROLLER_POLICY_OFF,
        ELM_SCROLLER_POLICY_AUTO);
    evas_object_show(more_view.more_scroller);
    edje_object_part_swallow(more_view.layout,
        "candidate_more", more_view.more_scroller);

    more_view.more_table = elm_table_add(win);
    elm_table_padding_set(more_view.more_table, 0, 1);
    evas_object_show(more_view.more_table);
    elm_object_content_set(more_view.more_scroller, more_view.more_table);
}

void
EflMultiLineCandidate::make_view()
{
    view.layout = edje_object_add(evas_object_evas_get(win));
    int ret = edje_object_file_set(view.layout,
        CANDIDATE_EDJ_FILE_PATH, "candidate");
    if (!ret) {
        printf("error while loading candidate layout\n");
        throw "failed loading candidate layout.";
    }

    evas_object_resize(view.layout, 720, 84);
    evas_object_show(view.layout);

    view.table = elm_table_add(win);
    evas_object_show(view.table);
    edje_object_part_swallow(view.layout,
        "candidate_bar", view.table);
}

EflMultiLineCandidate::EflMultiLineCandidate(Evas_Object *win)
{
    this->win = win;
    make_view();
    make_more_view();
}

EflMultiLineCandidate::~EflMultiLineCandidate() {
}

void
EflMultiLineCandidate::show() {
    show_view();
}

void
EflMultiLineCandidate::hide() {
    hide_view();
    hide_more_view();
}

void
EflMultiLineCandidate::update(const vector<string> &vec_str)
{
    cur_candidates = vec_str;
    show_view();
    hide_more_view();

    elm_table_clear(view.table, EINA_TRUE);

    int num;
    if (vec_str.size() <= CANDIDATE_BAR_NUM) {
        num = vec_str.size();
    } else  {
        num = CANDIDATE_BAR_NUM-1; // reserve one for more btn
    }

    vector<string>::const_iterator it;
    for (it = vec_str.begin(); it != vec_str.end(); it++) {
        int i = it-vec_str.begin();
        if (i >= num) {
            break;
        }
        Evas_Object *item = get_candidate_item();
        edje_object_part_text_set (item, "candidate",
            it->c_str());

        elm_table_pack(view.table, item, 2*i, 0, 1, 1);
        Evas_Object *seperate = _get_seperate_line(win);
        elm_table_pack(view.table, seperate, 2*i+1, 0, 1, 1);
    }

    if (vec_str.size() > CANDIDATE_BAR_NUM) {
        Evas_Object *btn = get_candidate_more_button();
        elm_table_pack(view.table, btn,
            2*CANDIDATE_BAR_NUM, 0, 1, 1);
    }
}

void
EflMultiLineCandidate::rotate(int degree) {
    this->degree = degree;
    switch (degree) {
        case 0:
        case 180:
            evas_object_resize(view.layout, 720, 84);
            evas_object_resize(more_view.layout, 720, 444);
            break;
        case 90:
        case 270:
            evas_object_resize(view.layout, 1280, 84);
            evas_object_resize(more_view.layout, 1280, 444);
            break;
    }
    show_view();
}
