#pragma once
#include <queue>
#include <string>

/**
 * A helper class EventBus uses a deque underneath
 * to create an EventBus that manages to events that have occured or need to be
 * handled by the code when the main loop is busy performing action.
 */
template <typename T>
class EventBus {
    deque<T> events;

public:
    /**
     * Description: Checks if EventBus is empty.
     * Pre: None
     * Post: Returns true if EventBus is empty.
     */
    boolean empty();

    /**
     * Description: Return the current event that needs to be handled.
     * Pre: None
     * Post: Return Event that is first in EventBus.
     */
    Event current();

    /**
     * Description: Add an event that needs to be handled to EventBus.
     * Pre: None
     * Post: An event that needs to be processed will be added to EventBus.
     */
    void add(T e);

    /**
     * Description: Add an event that needs to be handled on first priority to EventBus.
     * Pre: None
     * Post: An event that needs to be processed immediatly will be added to EventBus in the front.
     */
    void sos(T e);

    /**
     * Description: Mark the current event as completed and remove from the event bus.
     * Pre: None
     * Post: The first event from the event bus will be removed.
     */
    void current_completed();
};