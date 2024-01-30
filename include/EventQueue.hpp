#pragma once

#include <cstdint>
#include <queue>

// TODO delete this useless abstraction

namespace CASK {

class EventQueue {

private:

    std::queue<__uint32_t> events;

public:

    void EnqueueEvent(__uint32_t event);
    __uint32_t DequeueEvent();
    bool IsEmpty();

};

} // namespace CASK
