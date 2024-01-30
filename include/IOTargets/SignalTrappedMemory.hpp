
#include <csetjmp>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cassert>
#include <string.h>

char* arena;
std::jmp_buf jbuf;
sigset_t oldmask;

void segfault_sigaction(int signal, siginfo_t *si, void *arg)
{
    // printf("Trapped apparent device transaction at HA=0x%016llx\n", (__uint64_t)si->si_addr);
    // sigprocmask(SIG_SETMASK, &oldmask, NULL);
    std::longjmp(jbuf, 1);
}

void set_up_signal_trapped_memory() {
    arena = (char*)mmap(NULL, 0x100000000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    printf("Mapped arena at HA=0x%016llx\n", (uint64_t)arena);
    assert(arena != MAP_FAILED);
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_sigaction;
    sa.sa_flags   = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigprocmask(SIG_SETMASK, NULL, &oldmask);
}

template<typename T>
void make_device_hole(T startAddress, T size) {
    long sysconf_page_size = sysconf(_SC_PAGE_SIZE);
    char* munmap_safe_address = arena+startAddress;
    unsigned pot = 0;
    long tmp = sysconf_page_size;
    while ((tmp & 1) == 0) {
        tmp >>= 1;
        pot++;
    }
    // if (sysconf_page_size) non-power-of-two page size, what??
    munmap_safe_address = (char*)((uint64_t)munmap_safe_address & ~((1ul << pot) - 1));
    size += arena+startAddress - munmap_safe_address;
    
    // Mark the region as inaccessible
    int err = mprotect(munmap_safe_address, size, PROT_NONE);
    assert(err == 0);

    printf("Make device hole at PA=0x%08x, ReqHA=0x%016llx, ProtAddr=0x%016llx, size=0x%08x, pagesize=0x%08x\n",
        (__uint32_t)startAddress, (__uint64_t)(arena+startAddress), (__uint64_t)munmap_safe_address, (__uint32_t)size, (__uint32_t)sysconf_page_size);
    // TODO we need to handle and report errors and maybe fix up the hole ranges in the caller / be smart
}

namespace CASK {

class SignalTrappedMemory final : public IOTarget {

public:

    virtual __uint32_t Read32(__uint32_t startAddress, __uint32_t size, char* buf) override { return TransactInternal<__uint32_t, AccessType::R>(startAddress, size, buf); }
    virtual __uint64_t Read64(__uint64_t startAddress, __uint64_t size, char* buf) override { return TransactInternal<__uint64_t, AccessType::R>(startAddress, size, buf); }
    virtual __uint128_t Read128(__uint128_t startAddress, __uint128_t size, char* buf) override { return TransactInternal<__uint128_t, AccessType::R>(startAddress, size, buf); }
    virtual __uint32_t Write32(__uint32_t startAddress, __uint32_t size, char* buf) override { return TransactInternal<__uint32_t, AccessType::W>(startAddress, size, buf); }
    virtual __uint64_t Write64(__uint64_t startAddress, __uint64_t size, char* buf) override { return TransactInternal<__uint64_t, AccessType::W>(startAddress, size, buf); }
    virtual __uint128_t Write128(__uint128_t startAddress, __uint128_t size, char* buf) override { return TransactInternal<__uint128_t, AccessType::R>(startAddress, size, buf); }
    virtual __uint32_t Fetch32(__uint32_t startAddress, __uint32_t size, char* buf) override { return TransactInternal<__uint32_t, AccessType::X>(startAddress, size, buf); }
    virtual __uint64_t Fetch64(__uint64_t startAddress, __uint64_t size, char* buf) override { return TransactInternal<__uint64_t, AccessType::X>(startAddress, size, buf); }
    virtual __uint128_t Fetch128(__uint128_t startAddress, __uint128_t size, char* buf) override { return TransactInternal<__uint128_t, AccessType::X>(startAddress, size, buf); }

private:

    template <typename T, AccessType accessType>
    inline T TransactInternal(T startAddress, T size, char* buf) {
        if constexpr (accessType != AccessType::W) {
            // if constexpr (sizeof(T) <= 8)
            //     printf("R HA=0x%016llx size 0x%016llx\n", (__uint64_t)arena+startAddress, (__uint64_t)size);
            memcpy(buf, arena+startAddress, size);
        } else {
            // if constexpr (sizeof(T) <= 8)
            //     printf("W HA=0x%016llx size 0x%016llx\n", (__uint64_t)arena+startAddress, (__uint64_t)size);
            memcpy(arena+startAddress, buf, size);
        }
        return size;
    }
};

}; // namespace CASK;
