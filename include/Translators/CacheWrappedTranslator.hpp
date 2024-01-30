#pragma once

#include <Translator.hpp>

template<typename XLEN_t, unsigned int cacheBits>
class CacheWrappedTranslator : public Translator<XLEN_t> {

private:

    struct CacheEntry {
        Translation<XLEN_t> translation;
    };

    Translator<XLEN_t>* translator;

    CacheEntry cacheR[1 << cacheBits];
    CacheEntry cacheW[1 << cacheBits];
    CacheEntry cacheX[1 << cacheBits];

public:

    CacheWrappedTranslator(Translator<XLEN_t>* targetTranslator) : translator(targetTranslator) {
        Clear();
    }

    virtual inline Translation<XLEN_t> TranslateRead(XLEN_t address) override {
        return TranslateInternal<IOVerb::Read>(address);
    }

    virtual inline Translation<XLEN_t> TranslateWrite(XLEN_t address) override {
        return TranslateInternal<IOVerb::Write>(address);
    }

    virtual inline Translation<XLEN_t> TranslateFetch(XLEN_t address) override {
        return TranslateInternal<IOVerb::Fetch>(address);
    }

    void Clear() {
        for (unsigned int i = 0; i < (1 << cacheBits); i++) {
            // TODO I think this is not strictly correct
            cacheR[i].translation.untranslated = ~(XLEN_t)0;
            cacheW[i].translation.untranslated = ~(XLEN_t)0;
            cacheX[i].translation.untranslated = ~(XLEN_t)0;
        }
    }

private:

    template<IOVerb verb>
    inline Translation<XLEN_t> TranslateInternal(XLEN_t address) {
        CacheEntry* cache = cacheX;
        if constexpr (verb == IOVerb::Read) {
            cache = cacheR;
        } else if constexpr (verb == IOVerb::Write) {
            cache = cacheW;
        }
        unsigned int index = (address >> 12) & ((1 << cacheBits) - 1);
        if (cache[index].translation.untranslated >> 12 != address >> 12) [[ unlikely ]]
            cache[index].translation = translator->template Translate<verb>(address);
        return cache[index].translation;
    }
};
