#pragma once

#include <Transactor.hpp>

// TODO: we may one day need to make a buffered version of this transactor where the transactions are all or nothing.
template <typename XLEN_t, unsigned int cacheBits>
class VirtToHostTransactor final : public Transactor<XLEN_t> {

private:

    HartState<XLEN_t> *state;
    CASK::IOTarget* target;
    DirectTransactor<XLEN_t> transactor;
    struct CacheEntry { char *hostPageStart; XLEN_t virtPageStart; XLEN_t validThrough; };
    CacheEntry cacheR[1 << cacheBits];
    CacheEntry cacheW[1 << cacheBits];
    CacheEntry cacheX[1 << cacheBits];

public:

    VirtToHostTransactor(CASK::IOTarget* ioTarget, HartState<XLEN_t> *hartState) :
        state(hartState), target(ioTarget), transactor(target) {} // TODO eliminate transactor and prefer direct IOTarget. Why did I ever separate these?
    virtual inline Transaction<XLEN_t> Read(XLEN_t startAddress, XLEN_t size, char* buf) override { return TransactInternal<CASK::AccessType::R>(startAddress, size, buf); }
    virtual inline Transaction<XLEN_t> Write(XLEN_t startAddress, XLEN_t size, char* buf) override { return TransactInternal<CASK::AccessType::W>(startAddress, size, buf); }
    virtual inline Transaction<XLEN_t> Fetch(XLEN_t startAddress, XLEN_t size, char* buf) override { return TransactInternal<CASK::AccessType::X>(startAddress, size, buf); }

    void Clear() {
        memset(cacheR, 0, sizeof(cacheR));
        memset(cacheW, 0, sizeof(cacheW));
        memset(cacheX, 0, sizeof(cacheX));
    }

private:

    template <CASK::AccessType accessType>
    inline Transaction<XLEN_t> TransactInternal(XLEN_t startAddress, XLEN_t size, char* buf) {
        CacheEntry* cache = cacheX;
        if constexpr (accessType == CASK::AccessType::R) {
            cache = cacheR;
        } else if constexpr (accessType == CASK::AccessType::W) {
            cache = cacheW;
        }
        XLEN_t endAddress = startAddress + size - 1;
        while (startAddress <= endAddress) {
            unsigned int index = (startAddress >> 12) & ((1 << cacheBits) - 1); // TODO assumes 4k pages, need flex for supers
            if (cache[index].virtPageStart >> 12 == startAddress >> 12) [[ likely ]] {
                XLEN_t chunkEndAddress = cache[index].validThrough >= endAddress ? endAddress : cache[index].validThrough;
                XLEN_t chunkSize = chunkEndAddress - startAddress + 1;
                char *chunkHostAddress = cache[index].hostPageStart + startAddress - cache[index].virtPageStart;
                if constexpr (accessType == CASK::AccessType::W) {
                    memcpy(chunkHostAddress, buf, chunkSize);
                } else {
                    memcpy(buf, chunkHostAddress, chunkSize);
                }
                buf += chunkSize;
                startAddress += chunkSize;
                continue;
            }
            Translation<XLEN_t> fresh_translation = TranslationAlgorithm<XLEN_t, accessType>(
                startAddress, &transactor, state->satp.ppn, state->satp.pagingMode,
                state->mstatus.mprv ? state->mstatus.mpp : state->privilegeMode,
                state->mstatus.mxr, state->mstatus.sum);
            if (fresh_translation.generatedTrap != RISCV::TrapCause::NONE) [[unlikely]] {
                return { fresh_translation.generatedTrap, 0 };
            }
            XLEN_t chunkEndAddress = fresh_translation.validThrough >= endAddress ? endAddress : fresh_translation.validThrough;
            XLEN_t chunkSize = chunkEndAddress - startAddress + 1;
            XLEN_t translatedChunkStart = fresh_translation.translated + startAddress - fresh_translation.untranslated;
            /*XLEN_t transferredSize = TODO, errors if mismatch?*/
            target->template Transact<XLEN_t, accessType>(translatedChunkStart, chunkSize, buf);
            if (target->hint) {
                XLEN_t offset = fresh_translation.untranslated - fresh_translation.virtPageStart;
                cache[index].hostPageStart = (char*)target->hint - offset;
                cache[index].virtPageStart = fresh_translation.virtPageStart;
                cache[index].validThrough = fresh_translation.validThrough;
            }
            buf += chunkSize;
            startAddress += chunkSize;
        }
        return { RISCV::TrapCause::NONE, size };
    }
};
