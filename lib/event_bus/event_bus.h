#pragma once
#include <queue>
#include <string>

template <typename T>
class EventBus : public std::deque<T> {

public:
    std::string to_string();
};