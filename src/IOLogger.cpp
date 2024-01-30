#include <IOTargets/IOLogger.hpp>

namespace CASK {

IOLogger::IOLogger(IOTarget* startTarget, std::ostream* startOStream) :
    target(startTarget),
    stream(startOStream) {
}

__uint32_t IOLogger::Read32(__uint32_t startAddress, __uint32_t size, char* dst) {
    target->Read32(startAddress, size, dst);
    return WriteLog<__uint32_t>("Read32", dst, startAddress, size);
}

__uint64_t IOLogger::Read64(__uint64_t startAddress, __uint64_t size, char* dst) {
    target->Read64(startAddress, size, dst);
    return WriteLog<__uint64_t>("Read64", dst, startAddress, size);
}

__uint128_t IOLogger::Read128(__uint128_t startAddress, __uint128_t size, char* dst) {
    target->Read128(startAddress, size, dst);
    return WriteLog<__uint128_t>("Read128", dst, startAddress, size);
}

__uint32_t IOLogger::Write32(__uint32_t startAddress, __uint32_t size, char* src) {
    target->Write32(startAddress, size, src);
    return WriteLog<__uint32_t>("Write32", src, startAddress, size);
}

__uint64_t IOLogger::Write64(__uint64_t startAddress, __uint64_t size, char* src) {
    target->Write64(startAddress, size, src);
    return WriteLog<__uint64_t>("Write64", src, startAddress, size);
}

__uint128_t IOLogger::Write128(__uint128_t startAddress, __uint128_t size, char* src) {
    target->Write128(startAddress, size, src);
    return WriteLog<__uint128_t>("Write128", src, startAddress, size);
}

__uint32_t IOLogger::Fetch32(__uint32_t startAddress, __uint32_t size, char* dst) {
    target->Fetch32(startAddress, size, dst);
    return WriteLog<__uint32_t>("Fetch32", dst, startAddress, size);
}

__uint64_t IOLogger::Fetch64(__uint64_t startAddress, __uint64_t size, char* dst) {
    target->Fetch64(startAddress, size, dst);
    return WriteLog<__uint64_t>("Fetch64", dst, startAddress, size);
}

__uint128_t IOLogger::Fetch128(__uint128_t startAddress, __uint128_t size, char* dst) {
    target->Fetch128(startAddress, size, dst);
    return WriteLog<__uint128_t>("Fetch128", dst, startAddress, size);
}

void IOLogger::SetPrintContents(bool enabled) {
    logContents = enabled;
}

} // namespace CASK
