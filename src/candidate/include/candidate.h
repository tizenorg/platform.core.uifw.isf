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

#ifndef __CANDIDATE_H__
#define __CANDIDATE_H__
#include <vector>
#include <string>

class EventDesc
{
    public:
        virtual ~EventDesc() {}
        enum Event_Type{
            CANDIDATE_ITEM_MOUSE_DOWN = 0,
            CANDIDATE_ITEM_MOUSE_UP = 1,
            CANDIDATE_ITEM_MOUSE_MOVE = 2,
        };

        Event_Type type;
        int index;
        std::string text;
};

// the MultiEventDesc is for the candidate that has more
// button
class MultiEventDesc: public EventDesc
{
    public:
        enum Event_Type{
            CANDIDATE_ITEM_MOUSE_DOWN = 0,
            CANDIDATE_ITEM_MOUSE_UP = 1,
            CANDIDATE_ITEM_MOUSE_MOVE = 2,
            CANDIDATE_MORE_BTN_CLICKED = 3,
            CANDIDATE_MORE_BTN_RELEASED = 4,
            CANDIDATE_MORE_VIEW_SHOW = 5,
            CANDIDATE_MORE_VIEW_HIDE = 6,
            CANDIDATE_CLOSE_BTN_CLICKED = 7,
        };
        Event_Type type;
        int index;
        std::string text;
};

class EventListener
{
    public:
        virtual ~EventListener() { }
        virtual void on_event(const EventDesc &desc) = 0;
};

class Candidate
{
    public:
        Candidate() { m_visible = false; }
        virtual ~Candidate() { m_visible = false; }
        virtual void show() = 0;
        virtual void hide() = 0;
        virtual void update(const std::vector<std::string> &candidates) = 0;
        virtual void rotate(int degree) { }
        virtual int get_height() { return 0; }
        virtual bool get_visible() { return m_visible; }
        void add_event_listener(EventListener *l);
    protected:
        void notify_listeners(const EventDesc &desc);
        std::vector<EventListener*> listeners;

    protected:
        bool m_visible;
};
#endif
