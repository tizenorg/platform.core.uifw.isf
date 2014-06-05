#include "candidate.h"
using namespace std;

void
Candidate::add_event_listener(EventListener *l)
{
    listeners.push_back(l);
}

void
Candidate::notify_listeners(const EventDesc &desc)
{
    vector<EventListener*>::iterator it;
    for (it = listeners.begin(); it != listeners.end(); ++it) {
        EventListener *l = *it;
        l->on_event(desc);
    }
}
