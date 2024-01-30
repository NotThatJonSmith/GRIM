#pragma once

#include <HartState.hpp>
#include <Translator.hpp>
#include <Transactor.hpp>

#include <Tickable.hpp>

template<typename XLEN_t>
class Hart : public CASK::Tickable {

public:

    Hart(__uint32_t maximalExtensions) : state(maximalExtensions) { }
    
    virtual inline unsigned int Tick() override = 0;
    virtual inline void Reset() override { }
    virtual  inline Transactor<XLEN_t>* getVATransactor() = 0; // TODO this should be something more private

    HartState<XLEN_t> state;
    XLEN_t resetVector;

};
