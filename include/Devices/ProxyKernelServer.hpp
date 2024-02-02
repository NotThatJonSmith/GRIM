#pragma once

#include <Device.hpp>

#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <queue>
#include <filesystem>
#include <fcntl.h>

class ProxyKernelServer : public Device {
private:

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

    typedef __uint64_t (ProxyKernelServer::*SysCallHandler)(__uint64_t, __uint64_t, __uint64_t, __uint64_t, __uint64_t, __uint64_t, __uint64_t);
    struct SystemCall { SysCallHandler handler; std::string name; };
    std::unordered_map<int, SystemCall> syscallTable = {
        { SYS_exit, { &ProxyKernelServer::sys_exit, "exit" } },
        { SYS_exit_group, { &ProxyKernelServer::sys_exit_group, "exit_group" } },
        { SYS_getpid, { &ProxyKernelServer::sys_getpid, "getpid" } },
        { SYS_kill, { &ProxyKernelServer::sys_kill, "kill" } },
        { SYS_read, { &ProxyKernelServer::sys_read, "read" } },
        { SYS_write, { &ProxyKernelServer::sys_write, "write" } },
        { SYS_openat, { &ProxyKernelServer::sys_openat, "openat" } },
        { SYS_close, { &ProxyKernelServer::sys_close, "close" } },
        { SYS_lseek, { &ProxyKernelServer::sys_lseek, "lseek" } },
        { SYS_brk, { &ProxyKernelServer::sys_brk, "brk" } },
        { SYS_linkat, { &ProxyKernelServer::sys_linkat, "linkat" } },
        { SYS_unlinkat, { &ProxyKernelServer::sys_unlinkat, "unlinkat" } },
        { SYS_mkdirat, { &ProxyKernelServer::sys_mkdirat, "mkdirat" } },
        { SYS_renameat, { &ProxyKernelServer::sys_renameat, "renameat" } },
        { SYS_chdir, { &ProxyKernelServer::sys_chdir, "chdir" } },
        { SYS_getcwd, { &ProxyKernelServer::sys_getcwd, "getcwd" } },
        { SYS_fstat, { &ProxyKernelServer::sys_fstat, "fstat" } },
        { SYS_fstatat, { &ProxyKernelServer::sys_fstatat, "fstatat" } },
        { SYS_faccessat, { &ProxyKernelServer::sys_faccessat, "faccessat" } },
        { SYS_pread, { &ProxyKernelServer::sys_pread, "pread" } },
        { SYS_pwrite, { &ProxyKernelServer::sys_pwrite, "pwrite" } },
        { SYS_uname, { &ProxyKernelServer::sys_uname, "uname" } },
        { SYS_getuid, { &ProxyKernelServer::sys_getuid, "getuid" } },
        { SYS_geteuid, { &ProxyKernelServer::sys_geteuid, "geteuid" } },
        { SYS_getgid, { &ProxyKernelServer::sys_getgid, "getgid" } },
        { SYS_getegid, { &ProxyKernelServer::sys_getegid, "getegid" } },
        { SYS_mmap, { &ProxyKernelServer::sys_mmap, "mmap" } },
        { SYS_munmap, { &ProxyKernelServer::sys_munmap, "munmap" } },
        { SYS_mremap, { &ProxyKernelServer::sys_mremap, "mremap" } },
        { SYS_mprotect, { &ProxyKernelServer::sys_mprotect, "mprotect" } },
        { SYS_prlimit64, { &ProxyKernelServer::sys_prlimit64, "prlimit64" } },
        { SYS_getmainvars, { &ProxyKernelServer::sys_getmainvars, "getmainvars" } },
        { SYS_rt_sigaction, { &ProxyKernelServer::sys_rt_sigaction, "rt_sigaction" } },
        { SYS_writev, { &ProxyKernelServer::sys_writev, "writev" } },
        { SYS_gettimeofday, { &ProxyKernelServer::sys_gettimeofday, "gettimeofday" } },
        { SYS_times, { &ProxyKernelServer::sys_times, "times" } },
        { SYS_fcntl, { &ProxyKernelServer::sys_fcntl, "fcntl" } },
        { SYS_ftruncate, { &ProxyKernelServer::sys_ftruncate, "ftruncate" } },
        { SYS_getdents, { &ProxyKernelServer::sys_getdents, "getdents" } },
        { SYS_dup, { &ProxyKernelServer::sys_dup, "dup" } },
        { SYS_dup3, { &ProxyKernelServer::sys_dup3, "dup3" } },
        { SYS_readlinkat, { &ProxyKernelServer::sys_readlinkat, "readlinkat" } },
        { SYS_rt_sigprocmask, { &ProxyKernelServer::sys_rt_sigprocmask, "rt_sigprocmask" } },
        { SYS_ioctl, { &ProxyKernelServer::sys_ioctl, "ioctl" } },
        { SYS_getrlimit, { &ProxyKernelServer::sys_getrlimit, "getrlimit" } },
        { SYS_setrlimit, { &ProxyKernelServer::sys_setrlimit, "setrlimit" } },
        { SYS_getrusage, { &ProxyKernelServer::sys_getrusage, "getrusage" } },
        { SYS_clock_gettime, { &ProxyKernelServer::sys_clock_gettime, "clock_gettime" } },
        { SYS_set_tid_address, { &ProxyKernelServer::sys_set_tid_address, "set_tid_address" } },
        { SYS_set_robust_list, { &ProxyKernelServer::sys_set_robust_list, "set_robust_list" } },
        { SYS_madvise, { &ProxyKernelServer::sys_madvise, "madvise" } },
        { SYS_open, { &ProxyKernelServer::sys_open, "open" } },
        { SYS_link, { &ProxyKernelServer::sys_link, "link" } },
        { SYS_unlink, { &ProxyKernelServer::sys_unlink, "unlink" } },
        { SYS_mkdir, { &ProxyKernelServer::sys_mkdir, "mkdir" } },
        { SYS_access, { &ProxyKernelServer::sys_access, "access" } },
        { SYS_stat, { &ProxyKernelServer::sys_stat, "stat" } },
        { SYS_lstat, { &ProxyKernelServer::sys_lstat, "lstat" } },
        { SYS_time, { &ProxyKernelServer::sys_time, "time" } }
    };

public:

    ProxyKernelServer(Device *systemBus, std::queue<unsigned int> *eq, __uint32_t shutdownEventNumber) : bus(systemBus), events(eq), shutdownEvent(shutdownEventNumber) { }

    virtual __uint32_t Read32(__uint32_t startAddress, __uint32_t size, char* dst) override {
        // TODO proper bounds checks
        for (unsigned int i = 0; i < size; i++) {
            dst[i] = state[startAddress+i];
        }
        return size;
    }
    virtual __uint64_t Read64(__uint64_t startAddress, __uint64_t size, char* dst) override { return Read32(startAddress & 0xffffffff, size, dst); }
    virtual __uint128_t Read128(__uint128_t startAddress, __uint128_t size, char* dst) override { return Read32(startAddress & 0xffffffff, size, dst); }
    virtual __uint32_t Write32(__uint32_t startAddress, __uint32_t size, char* src) override {
        // Validate and copy the transaction into the state array
        // TODO proper bounds checks
        for (unsigned int i = 0; i < size; i++) {
            state[startAddress+i] = src[i];
        }
        // This happens when the final byte of the state array is written
        if (startAddress + size >= 16) {
            DispatchSystemCall(*((__uint64_t*)&state[8]));
            *((__uint64_t*)&state[8]) = 0;
            *((__uint64_t*)&state[0]) = 1;
        }
        return size;
    }
    virtual __uint64_t Write64(__uint64_t startAddress, __uint64_t size, char* src) override { return Write32(startAddress & 0xffffffff, size, src); }
    virtual __uint128_t Write128(__uint128_t startAddress, __uint128_t size, char* src) override { return Write32(startAddress & 0xffffffff, size, src); }
    virtual __uint32_t Fetch32(__uint32_t startAddress, __uint32_t size, char* dst) override { return 0; }
    virtual __uint64_t Fetch64(__uint64_t startAddress, __uint64_t size, char* dst) override { return 0; }
    virtual __uint128_t Fetch128(__uint128_t startAddress, __uint128_t size, char* dst) override { return 0; }

    void AttachLog(std::ostream* log) {
        out = log;
    }

    void SetFSRoot(std::string fsRoot) {
        fileSystemRoot = fsRoot;
    }

    void SetPipes(std::istream *stdin,
                                    std::ostream *stdout,
                                    std::ostream *stderr) {
        sim_stdin = stdin;
        sim_stdout = stdout;
        sim_stderr = stderr;
    }

    void SetCommandLine(std::string commandLine) {
        proxyKernelCommandLine = commandLine;
    }

private:

    Device *bus;
    std::queue<unsigned int> *events;

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

private:

    void DispatchSystemCall(__uint64_t magic_mem) {

        union SystemCallRequestUnion {
            struct SystemCallRequestStruct {
                __uint64_t n;
                __uint64_t args[7];
            } words;
            char bytes[64];
        } request;

        bus->Read64(magic_mem+sizeof(__uint64_t)*0, sizeof(request), request.bytes);

        __uint64_t result = -ERR_enosys;
        if (syscallTable.find(request.words.n) != syscallTable.end()) {
            SysCallHandler h = syscallTable[request.words.n].handler;
            result = (this->*h)(request.words.args[0], request.words.args[1],
                                request.words.args[2], request.words.args[3],
                                request.words.args[4], request.words.args[5],
                                request.words.args[6]);
            if (out) {
                (*out) << std::dec;
                (*out) << "Calling system call " << syscallTable[request.words.n].name
                        << "("
                        << request.words.args[0] << ", " << request.words.args[1] << ", "
                        << request.words.args[2] << ", " << request.words.args[3] << ", "
                        << request.words.args[4] << ", " << request.words.args[5] << ", "
                        << request.words.args[6] << ") returns "
                        << result << std::endl;
            }
        } else {
            if (out) {
                (*out) << std::dec;
                (*out) << "Calling unknown system call with code" << request.words.n
                    << std::endl;
            }
        }

        bus->Write64(magic_mem, sizeof(result), (char*)&result);
    }

    __uint64_t registerFD(std::fstream* fs) {

        __uint64_t fd;

        if (!fdFreePool.empty()) {
            fd = fdFreePool.back();
            fdFreePool.pop_back();
            return fd;
        } else {
            fd = nextFD;
            nextFD += 1;
        }

        openFiles.insert({fd, fs});
        return fd;
    }

    void releaseFD(__uint64_t fd) {
        std::fstream *fs = resolveFD(fd);
        if (fs == nullptr)
            return;
        openFiles.erase(fd);
        fdFreePool.push_back(fd);
    }

    std::fstream* resolveFD(__uint64_t fd) {
        if (fd <= 2 || openFiles.find(fd) == openFiles.end())
            return nullptr;
        return openFiles[fd];
    }

    std::istream* istreamFromFD(__uint64_t fd) {
        if (fd == 0)
            return sim_stdin;
        if (openFiles.find(fd) == openFiles.end())
            return nullptr;
        return openFiles[fd];
    }

    std::ostream* ostreamFromFD(__uint64_t fd) {
        if (fd == 1)
            return sim_stdout;
        if (fd == 2)
            return sim_stderr;
        if (openFiles.find(fd) == openFiles.end())
            return nullptr;
        return openFiles[fd];
    }
    __uint64_t sys_exit(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        events->push(shutdownEvent);
        return 0;
    }

    __uint64_t sys_exit_group(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_getpid(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_kill(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_read(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_write(__uint64_t fd, __uint64_t addr, __uint64_t size, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        std::ostream *fs = ostreamFromFD(fd);
        if (fs == nullptr)
            return -1; // TODO errno
        char *buf = new char[size];
        bus->Read64(addr, size, buf);
        fs->write(buf, size);
        delete[] buf;
        return size;
    }

    __uint64_t sys_openat(__uint64_t dirfd, __uint64_t fn, __uint64_t fn_size, __uint64_t flags, __uint64_t mode, __uint64_t arg5, __uint64_t arg6) {

        char *fileName = new char[fn_size];
        bus->Read64(fn, fn_size, fileName);

        std::filesystem::path filePath = std::filesystem::canonical(
            std::filesystem::path(
                fileSystemRoot + "/" + std::string(fileName)
            )
        );

        // TODO ensure that the canonical path is under the FS root.

        std::fstream * newStream;
        std::fstream::openmode fsMode = std::fstream::in;
        if ((flags & 3) == O_RDONLY) {
            fsMode = std::fstream::in;
        } else if ((flags & 3) == O_WRONLY) {
            fsMode = std::fstream::out;
        } else if ((flags & 3) == O_RDWR) {
            fsMode = std::fstream::in | std::fstream::out;
        } else {
            // TODO some error message or the special POSIX case here
        }

        // TODO other file flags/modes/creat etc...
        newStream = new std::fstream(filePath, std::fstream::binary | fsMode);

        return registerFD(newStream);
    }

    __uint64_t sys_close(__uint64_t fd, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        if (fd <= 2)
            return -1; // TODO errno... or maybe this isn't an error. Whatever.
        std::fstream *fs = resolveFD(fd);
        if (fs == nullptr)
            return -1; // TODO errno
        fs->close();
        delete fs;
        releaseFD(fd);
        return 0;
    }

    __uint64_t sys_lseek(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_brk(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_linkat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_unlinkat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_mkdirat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_renameat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_chdir(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_getcwd(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_fstat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_fstatat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_faccessat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_pread(__uint64_t fd, __uint64_t addr, __uint64_t size,
                            __uint64_t offset, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        if (fd <= 2)
            return -1; // TODO errno... or maybe this isn't an error. Whatever.
            // TODO use istreamFromFD instead to accommodate pread from stdin

        std::fstream *fs = resolveFD(fd);
        if (fs == nullptr)
            return -1; // TODO errno

        char *buf = new char[size];

        std::streampos bookmark = fs->tellg();
        fs->seekg(offset);
        fs->read(buf, size);
        __uint64_t count = *fs ? size : (__uint64_t)fs->gcount();
        fs->seekg(bookmark);

        bus->Write64(addr, count, buf);
        delete[] buf;
        return count;
    }

    __uint64_t sys_pwrite(__uint64_t fd, __uint64_t addr, __uint64_t size,
                            __uint64_t offset, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        if (fd <= 2)
            return -1; // TODO errno... or maybe this isn't an error. Whatever
            // TODO use ostreamFromFD instead to accommodate pwrite to stdout

        std::fstream *fs = resolveFD(fd);
        if (fs == nullptr)
            return -1; // TODO errno

        char *buf = new char[size];

        bus->Read64(addr, size, buf);

        std::streampos bookmark = fs->tellp();
        fs->seekp(offset, std::ios::beg);
        fs->write(buf, size);
        fs->seekp(bookmark, std::ios::beg);

        delete[] buf;
        return size;
    }

    __uint64_t sys_uname(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_getuid(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_geteuid(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_getgid(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_getegid(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_mmap(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_munmap(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_mremap(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_mprotect(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_prlimit64(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_getmainvars(__uint64_t addr, __uint64_t size, __uint64_t arg2, __uint64_t arg3,  __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    
        std::vector<unsigned int> argOffsets;
        bool spacePreceded = true;
        for (unsigned int i = 0; i < proxyKernelCommandLine.size(); i++) {
            if (proxyKernelCommandLine[i] == ' ') {
                spacePreceded = true;
                continue;
            } else if (spacePreceded) {
                argOffsets.push_back(i);
            }
            spacePreceded = false;
        }

        __uint64_t pk_argc = argOffsets.size();
        __uint64_t stringOffset = sizeof(__uint64_t) * (pk_argc + 1);
        __uint64_t requiredSpace = stringOffset + proxyKernelCommandLine.size() + 1;
        
        if (size < requiredSpace) {
            return requiredSpace - size;
        }

        __uint64_t *expectedArgStruct = new __uint64_t[size/sizeof(__uint64_t)];
        expectedArgStruct[0] = pk_argc;

        for (unsigned int i = 1; i < pk_argc+1; i++) {
            expectedArgStruct[i] = addr+stringOffset+argOffsets[i-1];
        }

        char* stringSection = (char*)&expectedArgStruct[pk_argc+1];
        for (unsigned int i = 0; i < proxyKernelCommandLine.size(); i++) {
            if (proxyKernelCommandLine[i] == ' ') {
                stringSection[i] = 0;
            } else {
                stringSection[i] = proxyKernelCommandLine[i];
            }
        }

        stringSection[proxyKernelCommandLine.size()] = 0;

        bus->Write64(addr, requiredSpace, (char*)&expectedArgStruct[0]);
        delete[] expectedArgStruct;
        return 0;
    }

    __uint64_t sys_rt_sigaction(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_writev(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_gettimeofday(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_times(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_fcntl(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_ftruncate(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_getdents(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_dup(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_dup3(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_readlinkat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_rt_sigprocmask(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_ioctl(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_getrlimit(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_setrlimit(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_getrusage(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_clock_gettime(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_set_tid_address(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_set_robust_list(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_madvise(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_open(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_link(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_unlink(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_mkdir(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_access(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_stat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_lstat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

    __uint64_t sys_time(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
        return -ERR_enosys;
    }

};
