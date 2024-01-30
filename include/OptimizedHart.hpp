#pragma once

#include <type_traits>
#include <cstdint>

#include <Hart.hpp>
#include <Decoders/PrecomputedDecoder.hpp>
#include <Transactors/VirtToHostTransactor.hpp>

// TODO accelerated WFI states?
template<typename XLEN_t>
class OptimizedHart final : public Hart<XLEN_t> {

private:

    static constexpr unsigned int icacheBits = 14;
    static constexpr unsigned int virtHostCacheBits = 8;

    PrecomputedDecoder<XLEN_t> decoder;
    VirtToHostTransactor<XLEN_t, virtHostCacheBits> transactor;
    // TODO turn on bufferTransactions later on.

    struct SimplyCachedInstruction {
        XLEN_t full_pc;
        __uint32_t encoding = 0;
        DecodedInstruction<XLEN_t> instruction = nullptr;
    };
    SimplyCachedInstruction icache[1<<icacheBits];

public:

    OptimizedHart(CASK::IOTarget* bus, __uint32_t maximalExtensions) :
        Hart<XLEN_t>(maximalExtensions),
        decoder(&this->state),
        transactor(bus, &this->state) {
        this->state.implCallback = std::bind(&OptimizedHart::Callback, this, std::placeholders::_1);
        // TODO callback for changing XLENs
        Reset();
    };

    virtual inline unsigned int Tick() override {
        for (unsigned int i = 0; i < 10000; i++) {
            SimplyCachedInstruction *inst = &icache[(this->state.pc >> 1) & ((1<<icacheBits)-1)];
            if (inst->full_pc == this->state.pc) [[ likely ]] {
                inst->instruction(inst->encoding, &this->state, &transactor);
                continue;
            }
            __uint32_t encoding;
            Transaction<XLEN_t> transaction = transactor.Fetch(this->state.pc, sizeof(encoding), (char*)&encoding);
            if (transaction.trapCause == RISCV::TrapCause::NONE) {
                DecodedInstruction<XLEN_t> decoded = decoder.Decode(encoding);
                icache[(this->state.pc >> 1) & ((1<<icacheBits)-1)] = { this->state.pc, encoding, decoded };
                decoded(encoding, &this->state, &transactor);
            } else {
                this->state.RaiseException(transaction.trapCause, this->state.pc);
            }
        }
        return 10000;
    };

    virtual inline void Reset() override {
        this->state.Reset(this->resetVector);
        transactor.Clear();
        decoder.Configure(&this->state);
        memset(icache, 0, sizeof(icache));
    };

    virtual inline Transactor<XLEN_t>* getVATransactor() override {
        return &this->transactor;
    }

private:

    inline void Callback(HartCallbackArgument arg) {
        if (arg == HartCallbackArgument::RequestedVMfence)
            transactor.Clear();
        if (arg == HartCallbackArgument::RequestedIfence || arg == HartCallbackArgument::RequestedVMfence)
            memset(icache, 0, sizeof(icache));
        if (arg == HartCallbackArgument::ChangedMISA) {
            memset(icache, 0, sizeof(icache));
            decoder.Configure(&this->state);
        }
        return;
    }

};
