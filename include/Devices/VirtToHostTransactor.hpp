#pragma once

#include <RiscVTranslationAlgorithm.hpp>

// TODO: we may one day need to make a buffered version of this where the transactions are all or nothing.
template <typename XLEN_t, unsigned int cacheBits>
class VirtToHostTransactor final : public Device {

private:

    HartState<XLEN_t> *state;
    Device* target;
    struct CacheEntry { char *hostPageStart; XLEN_t virtPageStart; XLEN_t validThrough; };
    CacheEntry cacheR[1 << cacheBits];
    CacheEntry cacheW[1 << cacheBits];
    CacheEntry cacheX[1 << cacheBits];

public:

    VirtToHostTransactor(Device* ioTarget, HartState<XLEN_t> *hartState) : state(hartState), target(ioTarget) {}
    virtual __uint32_t Read32(__uint32_t startAddress, __uint32_t size, char* buf) override { return TransactInternal<AccessType::R>(startAddress, size, buf); }
    virtual __uint64_t Read64(__uint64_t startAddress, __uint64_t size, char* buf) override { return TransactInternal<AccessType::R>(startAddress, size, buf); }
    virtual __uint128_t Read128(__uint128_t startAddress, __uint128_t size, char* buf) override { return TransactInternal<AccessType::R>(startAddress, size, buf); }
    virtual __uint32_t Write32(__uint32_t startAddress, __uint32_t size, char* buf) override { return TransactInternal<AccessType::W>(startAddress, size, buf); }
    virtual __uint64_t Write64(__uint64_t startAddress, __uint64_t size, char* buf) override { return TransactInternal<AccessType::W>(startAddress, size, buf); }
    virtual __uint128_t Write128(__uint128_t startAddress, __uint128_t size, char* buf) override { return TransactInternal<AccessType::W>(startAddress, size, buf); }
    virtual __uint32_t Fetch32(__uint32_t startAddress, __uint32_t size, char* buf) override { return TransactInternal<AccessType::X>(startAddress, size, buf); }
    virtual __uint64_t Fetch64(__uint64_t startAddress, __uint64_t size, char* buf) override { return TransactInternal<AccessType::X>(startAddress, size, buf); }
    virtual __uint128_t Fetch128(__uint128_t startAddress, __uint128_t size, char* buf) override { return TransactInternal<AccessType::X>(startAddress, size, buf); }

    void Clear() {
        memset(cacheR, 0, sizeof(cacheR));
        memset(cacheW, 0, sizeof(cacheW));
        memset(cacheX, 0, sizeof(cacheX));
    }

private:

    template <AccessType accessType>
    inline XLEN_t TransactInternal(XLEN_t startAddress, XLEN_t size, char* buf) {
        CacheEntry* cache = cacheX;
        if constexpr (accessType == AccessType::R) {
            cache = cacheR;
        } else if constexpr (accessType == AccessType::W) {
            cache = cacheW;
        }
        XLEN_t endAddress = startAddress + size - 1;
        XLEN_t chunkStartAddress = startAddress;
        while (chunkStartAddress <= endAddress) {
            unsigned int index = (chunkStartAddress >> 12) & ((1 << cacheBits) - 1); // TODO assumes 4k pages, need flex for supers
            if (cache[index].virtPageStart >> 12 == chunkStartAddress >> 12) [[ likely ]] {
                XLEN_t chunkEndAddress = cache[index].validThrough >= endAddress ? endAddress : cache[index].validThrough;
                XLEN_t chunkSize = chunkEndAddress - chunkStartAddress + 1;
                char *chunkHostAddress = cache[index].hostPageStart + chunkStartAddress - cache[index].virtPageStart;
                if constexpr (accessType == AccessType::W) {
                    memcpy(chunkHostAddress, buf, chunkSize);
                } else {
                    memcpy(buf, chunkHostAddress, chunkSize);
                }
                buf += chunkSize;
                chunkStartAddress += chunkSize;
                continue;
            }
            Translation<XLEN_t> fresh_translation = TranslationAlgorithm<XLEN_t, accessType>(
                chunkStartAddress, target, state->satp.ppn, state->satp.pagingMode,
                state->mstatus.mprv ? state->mstatus.mpp : state->privilegeMode,
                state->mstatus.mxr, state->mstatus.sum);
            if (fresh_translation.generatedTrap != RISCV::TrapCause::NONE) [[unlikely]] {
                state->RaiseException(fresh_translation.generatedTrap, startAddress);
                return 0; // TODO this is a lie; how much was really transacted? Latent bug here I'm just lazy now.
            }
            XLEN_t chunkEndAddress = fresh_translation.validThrough >= endAddress ? endAddress : fresh_translation.validThrough;
            XLEN_t chunkSize = chunkEndAddress - chunkStartAddress + 1;
            XLEN_t translatedChunkStart = fresh_translation.translated + chunkStartAddress - fresh_translation.untranslated;
            /*XLEN_t transferredSize = TODO, errors if mismatch?*/
            target->template Transact<XLEN_t, accessType>(translatedChunkStart, chunkSize, buf);
            if (target->hint) {
                XLEN_t offset = fresh_translation.untranslated - fresh_translation.virtPageStart;
                cache[index].hostPageStart = (char*)target->hint - offset;
                cache[index].virtPageStart = fresh_translation.virtPageStart;
                cache[index].validThrough = fresh_translation.validThrough;
            }
            buf += chunkSize;
            chunkStartAddress += chunkSize;
        }
        return size;
    }
};
