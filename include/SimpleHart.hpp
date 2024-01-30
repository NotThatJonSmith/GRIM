#pragma once

#include <type_traits>
#include <cstdint>

#include <Hart.hpp>

#include <Translators/DirectTranslator.hpp>
#include <Transactors/DirectTransactor.hpp>
#include <Transactors/TranslatingTransactor.hpp>
#include <Decoders/DirectDecoder.hpp>


template<typename XLEN_t>
class SimpleHart final : public Hart<XLEN_t> {

private:

    DirectTransactor<XLEN_t> paTransactor;
    DirectTranslator<XLEN_t> translator;
    TranslatingTransactor<XLEN_t, true> vaTransactor;
    DirectDecoder<XLEN_t> decoder;

public:

    SimpleHart(CASK::IOTarget* bus, __uint32_t maximalExtensions) :
        Hart<XLEN_t>(maximalExtensions),
        paTransactor(bus),
        translator(&this->state, &paTransactor),
        vaTransactor(&translator, &paTransactor),
        decoder(&this->state) {
        // TODO callback for changing XLEN!
        Reset();
    };

    virtual inline unsigned int Tick() override {

        __uint32_t encoding = 0;
        while (true) {
            Transaction<XLEN_t> transaction = vaTransactor.Fetch(this->state.pc, sizeof(encoding), (char*)&encoding);
            if (transaction.trapCause != RISCV::TrapCause::NONE) {
                this->state.RaiseException(transaction.trapCause, this->state.pc);
                continue;
            }
            break;
            // if (transaction.size != sizeof(encoding)) // TODO what if?
        }

        decoder.Decode(encoding)(encoding, &this->state, &vaTransactor);
        return 1;
    };

    virtual inline void Reset() override {
        this->state.Reset(this->resetVector);
    };

    virtual inline Transactor<XLEN_t>* getVATransactor() override {
        return &vaTransactor;
    }

};
