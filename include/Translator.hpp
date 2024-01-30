#pragma once

#include <Translation.hpp>
#include <IOTarget.hpp>

template<typename XLEN_t>
class Translator {
public:
    virtual inline Translation<XLEN_t> TranslateRead(XLEN_t address) = 0;
    virtual inline Translation<XLEN_t> TranslateWrite(XLEN_t address) = 0;
    virtual inline Translation<XLEN_t> TranslateFetch(XLEN_t address) = 0;
    template<CASK::AccessType accessType>
    inline Translation<XLEN_t> Translate(XLEN_t address) {
        if constexpr (accessType == CASK::AccessType::R) {
            return TranslateRead(address);
        } else if constexpr (accessType == CASK::AccessType::W) {
            return TranslateWrite(address);
        } else {
            return TranslateFetch(address);
        }
    }
};
