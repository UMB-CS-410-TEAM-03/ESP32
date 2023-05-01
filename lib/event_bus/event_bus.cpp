#include "event_bus.h"

template <typename T>
std::string EventBus<T>::to_string()
{
    std::string serialized = "[";
    for (auto it = this->begin(), end = this->end(); it != end; ++it) {
        auto e = *it;
        serialized += std::to_string(e);
        serialized += ',';
    }
    serialized += ']';

    return serialized;
}