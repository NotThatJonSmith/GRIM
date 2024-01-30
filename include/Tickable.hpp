#pragma once

namespace CASK {

class Tickable {

public:

    virtual inline unsigned int Tick() = 0;
    virtual inline void BeforeFirstTick() { };
    virtual inline void Reset() { };

};

} // namespace CASK
