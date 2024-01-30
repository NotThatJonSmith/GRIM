#pragma once

#include <IOTarget.hpp>
#include <EventQueue.hpp>

#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>

namespace CASK {

class ProxyKernelServer : public CASK::IOTarget {

public:

    ProxyKernelServer(IOTarget *systemBus, EventQueue *eq, __uint32_t shutdownEventNumber);
    virtual __uint32_t Read32(__uint32_t startAddress, __uint32_t size, char* dst) override;
    virtual __uint64_t Read64(__uint64_t startAddress, __uint64_t size, char* dst) override;
    virtual __uint128_t Read128(__uint128_t startAddress, __uint128_t size, char* dst) override;
    virtual __uint32_t Write32(__uint32_t startAddress, __uint32_t size, char* src) override;
    virtual __uint64_t Write64(__uint64_t startAddress, __uint64_t size, char* src) override;
    virtual __uint128_t Write128(__uint128_t startAddress, __uint128_t size, char* src) override;
    virtual __uint32_t Fetch32(__uint32_t startAddress, __uint32_t size, char* src) override;
    virtual __uint64_t Fetch64(__uint64_t startAddress, __uint64_t size, char* src) override;
    virtual __uint128_t Fetch128(__uint128_t startAddress, __uint128_t size, char* src) override;
    void SetFSRoot(std::string fsRoot);
    void SetPipes(std::istream *stdin, std::ostream *stdout, std::ostream *stderr);
    void SetCommandLine(std::string commandLine);
    void AttachLog(std::ostream* log);

private:

    IOTarget *bus;
    EventQueue *events;

    __uint8_t state[16];

    std::string fileSystemRoot = ".";
    std::string proxyKernelCommandLine = "";
    __uint32_t shutdownEvent;

    std::istream *sim_stdin = &std::cin;
    std::ostream *sim_stdout = &std::cout;
    std::ostream *sim_stderr = &std::cerr;

    std::ostream *out = nullptr;

    __uint64_t nextFD = 3;
    std::vector<__uint64_t> fdFreePool;
    std::unordered_map<__uint64_t, std::fstream*> openFiles;

    typedef __uint64_t (ProxyKernelServer::*SysCallHandler)(__uint64_t, __uint64_t,
        __uint64_t, __uint64_t, __uint64_t, __uint64_t, __uint64_t);
    struct SystemCall {
        SysCallHandler handler;
        std::string name;
    };
    static std::unordered_map<int, SystemCall> syscallTable;

private:

    void DispatchSystemCall(__uint64_t magic_mem);

    __uint64_t registerFD(std::fstream* fs);
    void releaseFD(__uint64_t fd);
    std::fstream* resolveFD(__uint64_t fd);
    std::ostream* ostreamFromFD(__uint64_t fd);
    std::istream* istreamFromFD(__uint64_t fd);

    static constexpr int
        SYS_exit = 93, SYS_exit_group = 94, SYS_getpid = 172, SYS_kill = 129,
        SYS_read = 63, SYS_write = 64, SYS_openat = 56, SYS_close = 57,
        SYS_lseek = 62, SYS_brk = 214, SYS_linkat = 37, SYS_unlinkat = 35,
        SYS_mkdirat = 34, SYS_renameat = 38, SYS_chdir = 49, SYS_getcwd = 17,
        SYS_fstat = 80, SYS_fstatat = 79, SYS_faccessat = 48, SYS_pread = 67,
        SYS_pwrite = 68, SYS_uname = 160, SYS_getuid = 174, SYS_geteuid = 175,
        SYS_getgid = 176, SYS_getegid = 177, SYS_mmap = 222, SYS_munmap = 215,
        SYS_mremap = 216, SYS_mprotect = 226, SYS_prlimit64 = 261,
        SYS_getmainvars = 2011, SYS_rt_sigaction = 134, SYS_writev = 66,
        SYS_gettimeofday = 169, SYS_times = 153, SYS_fcntl = 25,
        SYS_ftruncate = 46, SYS_getdents = 61, SYS_dup = 23, SYS_dup3 = 24,
        SYS_readlinkat = 78, SYS_rt_sigprocmask = 135, SYS_ioctl = 29,
        SYS_getrlimit = 163, SYS_setrlimit = 164, SYS_getrusage = 165,
        SYS_clock_gettime = 113, SYS_set_tid_address = 96,
        SYS_set_robust_list = 99, SYS_madvise = 233, SYS_open = 1024,
        SYS_link = 1025, SYS_unlink = 1026, SYS_mkdir = 1030, SYS_access = 1033,
        SYS_stat = 1038, SYS_lstat = 1039, SYS_time = 1062,
        ERR_eperm = 1, ERR_enoent = 2, ERR_esrch = 3, ERR_eintr = 4,
        ERR_eio = 5, ERR_enxio = 6, ERR_e2big = 7, ERR_enoexec = 8,
        ERR_ebadf = 9, ERR_echild = 10, ERR_eagain = 11, ERR_enomem = 12,
        ERR_eacces = 13, ERR_efault = 14, ERR_enotblk = 15, ERR_ebusy = 16,
        ERR_eexist = 17, ERR_exdev = 18, ERR_enodev = 19, ERR_enotdir = 20,
        ERR_eisdir = 21, ERR_einval = 22, ERR_enfile = 23, ERR_emfile = 24,
        ERR_enotty = 25, ERR_etxtbsy = 26, ERR_efbig = 27, ERR_enospc = 28,
        ERR_espipe = 29, ERR_erofs = 30, ERR_emlink = 31, ERR_epipe = 32,
        ERR_edom = 33, ERR_erange = 34, ERR_enomsg = 35, ERR_eidrm = 36,
        ERR_echrng = 37, ERR_el2nsync = 38, ERR_el3hlt = 39, ERR_el3rst = 40,
        ERR_elnrng = 41, ERR_eunatch = 42, ERR_enocsi = 43, ERR_el2hlt = 44,
        ERR_edeadlk = 45, ERR_enolck = 46, ERR_ebade = 50, ERR_ebadr = 51,
        ERR_exfull = 52, ERR_enoano = 53, ERR_ebadrqc = 54, ERR_ebadslt = 55,
        ERR_edeadlock = 56, ERR_ebfont = 57, ERR_enostr = 60, ERR_enodata = 61,
        ERR_etime = 62, ERR_enosr = 63, ERR_enonet = 64, ERR_enopkg = 65,
        ERR_eremote = 66, ERR_enolink = 67, ERR_eadv = 68, ERR_esrmnt = 69,
        ERR_ecomm = 70, ERR_eproto = 71, ERR_emultihop = 74, ERR_elbin = 75,
        ERR_edotdot = 76, ERR_ebadmsg = 77, ERR_eftype = 79, ERR_enotuniq = 80,
        ERR_ebadfd = 81, ERR_eremchg = 82, ERR_elibacc = 83, ERR_elibbad = 84,
        ERR_elibscn = 85, ERR_elibmax = 86, ERR_elibexec = 87, ERR_enosys = 88,
        ERR_enmfile = 89, ERR_enotempty = 90, ERR_enametoolong = 91,
        ERR_eloop = 92, ERR_eopnotsupp = 95, ERR_epfnosupport = 96,
        ERR_econnreset = 104, ERR_enobufs = 105, ERR_eafnosupport = 106,
        ERR_eprototype = 107, ERR_enotsock = 108, ERR_enoprotoopt = 109,
        ERR_eshutdown = 110, ERR_econnrefused = 111, ERR_eaddrinuse = 112,
        ERR_econnaborted = 113, ERR_enetunreach = 114, ERR_enetdown = 115,
        ERR_etimedout = 116, ERR_ehostdown = 117, ERR_ehostunreach = 118,
        ERR_einprogress = 119, ERR_ealready = 120, ERR_edestaddrreq = 121,
        ERR_emsgsize = 122, ERR_eprotonosupport = 123,
        ERR_esocktnosupport = 124, ERR_eaddrnotavail = 125, ERR_enetreset = 126,
        ERR_eisconn = 127, ERR_enotconn = 128, ERR_etoomanyrefs = 129,
        ERR_eproclim = 130, ERR_eusers = 131, ERR_edquot = 132,
        ERR_estale = 133, ERR_enotsup = 134, ERR_enomedium = 135,
        ERR_enoshare = 136, ERR_ecaseclash = 137, ERR_eilseq = 138,
        ERR_eoverflow = 139, ERR_ewouldblock = 11;

    __uint64_t sys_exit(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_exit_group(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_getpid(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_kill(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_read(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_write(__uint64_t fd, __uint64_t addr, __uint64_t size, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_openat(__uint64_t dirfd, __uint64_t fn, __uint64_t fn_size, __uint64_t flags, __uint64_t mode, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_close(__uint64_t fd, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_lseek(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_brk(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_linkat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_unlinkat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_mkdirat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_renameat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_chdir(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_getcwd(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_fstat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_fstatat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_faccessat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_pread(__uint64_t fd, __uint64_t addr, __uint64_t size, __uint64_t offset, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_pwrite(__uint64_t fd, __uint64_t addr, __uint64_t size, __uint64_t offset, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_uname(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_getuid(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_geteuid(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_getgid(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_getegid(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_mmap(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_munmap(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_mremap(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_mprotect(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_prlimit64(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_getmainvars(__uint64_t addr, __uint64_t size, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_rt_sigaction(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_writev(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_gettimeofday(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_times(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_fcntl(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_ftruncate(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_getdents(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_dup(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_dup3(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_readlinkat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_rt_sigprocmask(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_ioctl(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_getrlimit(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_setrlimit(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_getrusage(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_clock_gettime(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_set_tid_address(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_set_robust_list(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_madvise(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_open(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_link(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_unlink(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_mkdir(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_access(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_stat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_lstat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
    __uint64_t sys_time(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6);
};

} // namespace CASK
