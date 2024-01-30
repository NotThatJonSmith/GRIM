#pragma once

#include <queue>

#include <Transactor.hpp>
#include <Translator.hpp>

template <typename XLEN_t, bool bufferTransactions>
class TranslatingTransactor final : public Transactor<XLEN_t> {

private:

    Translator<XLEN_t>* translator;
    Transactor<XLEN_t>* transactor;

public:

    TranslatingTransactor(Translator<XLEN_t>* translator, Transactor<XLEN_t>* transactor) :
        translator(translator), transactor(transactor) {
    }

    virtual inline Transaction<XLEN_t> Read(XLEN_t startAddress, XLEN_t size, char* buf) override {
        return TransactInternal<CASK::AccessType::R>(startAddress, size, buf);
    }

    virtual inline Transaction<XLEN_t> Write(XLEN_t startAddress, XLEN_t size, char* buf) override {
        return TransactInternal<CASK::AccessType::W>(startAddress, size, buf);
    }

    virtual inline Transaction<XLEN_t> Fetch(XLEN_t startAddress, XLEN_t size, char* buf) override {
        return TransactInternal<CASK::AccessType::X>(startAddress, size, buf);
    }

private:

    template <CASK::AccessType accessType>
    inline Transaction<XLEN_t> TransactInternal(XLEN_t startAddress, XLEN_t size, char* buf) {
        if constexpr (bufferTransactions) {
            return TransactBuffered<accessType>(startAddress, size, buf);
        } else {
            return TransactImmediate<accessType>(startAddress, size, buf);
        }
    }

    template <CASK::AccessType accessType>
    inline Transaction<XLEN_t> TransactImmediate(XLEN_t startAddress, XLEN_t size, char* buf) {

        XLEN_t endAddress = startAddress + size - 1;
        if (endAddress < startAddress) {
            return { RISCV::TrapCause::NONE, 0 };
        }

        // TODO most of these will end up in mem, not bus. Can we cache an offset into the memory array and go faster?
        // TODO this function is the hot spot, and this page-striding behavior seems pointless.
        XLEN_t chunkStartAddress = startAddress;
        while (chunkStartAddress <= endAddress) {
            Translation<XLEN_t> translation = translator->template Translate<accessType>(chunkStartAddress);
            if (translation.generatedTrap != RISCV::TrapCause::NONE) [[unlikely]] {
                return { translation.generatedTrap, 0 };
            }
            XLEN_t chunkEndAddress = translation.validThrough;
            if (chunkEndAddress > endAddress) {
                chunkEndAddress = endAddress;
            }
            XLEN_t chunkSize = chunkEndAddress - chunkStartAddress + 1;
            char* chunkBuf = buf + (chunkStartAddress - startAddress);
            XLEN_t translatedChunkStart = translation.translated + chunkStartAddress - translation.untranslated;
            Transaction<XLEN_t> chunkResult = transactor->template Transact<accessType>(translatedChunkStart, chunkSize, chunkBuf);
            if (chunkResult.transferredSize != chunkSize) [[unlikely]] {
                return chunkResult;
            }
            chunkStartAddress += chunkSize;
        }
        return { RISCV::TrapCause::NONE, size };
    }

    template <CASK::AccessType accessType>
    inline Transaction<XLEN_t> TransactBuffered(XLEN_t startAddress, XLEN_t size, char* buf) {

        Transaction<XLEN_t> result;
        result.trapCause = RISCV::TrapCause::NONE;
        result.transferredSize = 0;

        XLEN_t endAddress = startAddress + size - 1;

        if (endAddress < startAddress) {
            return result;
        }

        struct BufferedTransaction {
            XLEN_t startAddress;
            XLEN_t size;
            char* buf;
        };
        static std::queue<BufferedTransaction> transactionQueue;

        XLEN_t chunkStartAddress = startAddress;
        while (chunkStartAddress <= endAddress) {

            Translation<XLEN_t> translation = translator->template Translate<accessType>(chunkStartAddress);

            result.trapCause = translation.generatedTrap;
            if (result.trapCause != RISCV::TrapCause::NONE) {
                while (!transactionQueue.empty()) {
                    transactionQueue.pop();
                }
                return result;
            }

            XLEN_t chunkEndAddress = translation.validThrough;
            if (chunkEndAddress > endAddress) {
                chunkEndAddress = endAddress;
            }
            XLEN_t chunkSize = chunkEndAddress - chunkStartAddress + 1;

            char* chunkBuf = buf + (chunkStartAddress - startAddress);
            XLEN_t translatedChunkStart = translation.translated + chunkStartAddress - translation.untranslated;
            transactionQueue.push({translatedChunkStart, chunkSize, chunkBuf});
            chunkStartAddress += chunkSize;

        }

        result.transferredSize = 0;
        while (!transactionQueue.empty()) {
            BufferedTransaction transaction = transactionQueue.front();
            transactionQueue.pop();
            Transaction<XLEN_t> chunkResult =
                transactor->template Transact<accessType>(transaction.startAddress, transaction.size, transaction.buf);
            result.transferredSize += chunkResult.transferredSize;
            if (chunkResult.transferredSize != transaction.size) {
                break;
            }
        }

        return result;
    }

};
