/**
 * Author: Malav Patel
 * Description: Purpose of this file is to implement the
 * EventBus Class defined in event_bus.h
 */
#include "event_bus.h"

template <typename T>
class EventBus {
    deque<T> events;

public:
    boolean empty()
    {
        return events.empty();
    }

    Event current()
    {
        return events.front();
    }

    void add(T e)
    {
        events.push_back(e);
    }

    void sos(T e)
    {
        events.push_front(e);
    }

    void current_completed()
    {
        events.pop_front();
    }
};