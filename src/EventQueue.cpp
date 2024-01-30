#include <EventQueue.hpp>

namespace CASK {

void EventQueue::EnqueueEvent(__uint32_t event) {
    events.push(event);
}

__uint32_t EventQueue::DequeueEvent() {
    __uint32_t event = 0;
    if (events.empty()) {
        return event;
    }
    event = events.front();
    events.pop();
    return event;
}

bool EventQueue::IsEmpty() {
    return events.empty();
}

} // namespace CASK