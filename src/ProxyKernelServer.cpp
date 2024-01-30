#include <IOTargets/ProxyKernelServer.hpp>

#include <filesystem>
#include <fcntl.h>

namespace CASK {

ProxyKernelServer::ProxyKernelServer(IOTarget *systemBus, EventQueue *eq, __uint32_t shutdownEventNumber) :
    bus(systemBus),
    events(eq),
    shutdownEvent(shutdownEventNumber) {
}

void ProxyKernelServer::AttachLog(std::ostream* log) {
    out = log;
}

__uint32_t ProxyKernelServer::Read32(__uint32_t startAddress, __uint32_t size, char* dst) {
    // TODO proper bounds checks
    for (unsigned int i = 0; i < size; i++) {
        dst[i] = state[startAddress+i];
    }
    return size;
}

__uint32_t ProxyKernelServer::Write32(__uint32_t startAddress, __uint32_t size, char* src) {

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

__uint64_t ProxyKernelServer::Write64(__uint64_t startAddress, __uint64_t size, char* src) {
    return Write32(startAddress & 0xffffffff, size, src);
}

__uint128_t ProxyKernelServer::Write128(__uint128_t startAddress, __uint128_t size, char* src) {
    return Write32(startAddress & 0xffffffff, size, src);
}

__uint64_t ProxyKernelServer::Read64(__uint64_t startAddress, __uint64_t size, char* dst) {
    return Read32(startAddress & 0xffffffff, size, dst);
}

__uint128_t ProxyKernelServer::Read128(__uint128_t startAddress, __uint128_t size, char* dst) {
    return Read32(startAddress & 0xffffffff, size, dst);
}

__uint32_t ProxyKernelServer::Fetch32(__uint32_t startAddress, __uint32_t size, char* dst) {
    return 0;
}

__uint64_t ProxyKernelServer::Fetch64(__uint64_t startAddress, __uint64_t size, char* dst) {
    return 0;
}

__uint128_t ProxyKernelServer::Fetch128(__uint128_t startAddress, __uint128_t size, char* dst) {
    return 0;
}

void ProxyKernelServer::DispatchSystemCall(__uint64_t magic_mem) {

    union SystemCallRequestUnion {
        struct SystemCallRequestStruct {
            __uint64_t n;
            __uint64_t args[7];
        } words;
        char bytes[64];
    } request;

    bus->Read64(magic_mem+sizeof(__uint64_t)*0, sizeof(request), request.bytes);

    __uint64_t result = -ProxyKernelServer::ERR_enosys;
    if (syscallTable.find(request.words.n) != syscallTable.end()) {
        ProxyKernelServer::SysCallHandler h = syscallTable[request.words.n].handler;
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

void ProxyKernelServer::SetFSRoot(std::string fsRoot) {
    fileSystemRoot = fsRoot;
}

void ProxyKernelServer::SetPipes(std::istream *stdin,
                                 std::ostream *stdout,
                                 std::ostream *stderr) {
    sim_stdin = stdin;
    sim_stdout = stdout;
    sim_stderr = stderr;
}

void ProxyKernelServer::SetCommandLine(std::string commandLine) {
    proxyKernelCommandLine = commandLine;
}

__uint64_t ProxyKernelServer::registerFD(std::fstream* fs) {

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

void ProxyKernelServer::releaseFD(__uint64_t fd) {
    std::fstream *fs = resolveFD(fd);
    if (fs == nullptr)
        return;
    openFiles.erase(fd);
    fdFreePool.push_back(fd);
}

std::fstream* ProxyKernelServer::resolveFD(__uint64_t fd) {
    if (fd <= 2 || openFiles.find(fd) == openFiles.end())
        return nullptr;
    return openFiles[fd];
}

std::istream* ProxyKernelServer::istreamFromFD(__uint64_t fd) {
    if (fd == 0)
        return sim_stdin;
    if (openFiles.find(fd) == openFiles.end())
        return nullptr;
    return openFiles[fd];
}

std::ostream* ProxyKernelServer::ostreamFromFD(__uint64_t fd) {
    if (fd == 1)
        return sim_stdout;
    if (fd == 2)
        return sim_stderr;
    if (openFiles.find(fd) == openFiles.end())
        return nullptr;
    return openFiles[fd];
}

std::unordered_map<int, ProxyKernelServer::SystemCall> ProxyKernelServer::syscallTable = {
    { ProxyKernelServer::SYS_exit, { &ProxyKernelServer::sys_exit, "exit" } },
    { ProxyKernelServer::SYS_exit_group, { &ProxyKernelServer::sys_exit_group, "exit_group" } },
    { ProxyKernelServer::SYS_getpid, { &ProxyKernelServer::sys_getpid, "getpid" } },
    { ProxyKernelServer::SYS_kill, { &ProxyKernelServer::sys_kill, "kill" } },
    { ProxyKernelServer::SYS_read, { &ProxyKernelServer::sys_read, "read" } },
    { ProxyKernelServer::SYS_write, { &ProxyKernelServer::sys_write, "write" } },
    { ProxyKernelServer::SYS_openat, { &ProxyKernelServer::sys_openat, "openat" } },
    { ProxyKernelServer::SYS_close, { &ProxyKernelServer::sys_close, "close" } },
    { ProxyKernelServer::SYS_lseek, { &ProxyKernelServer::sys_lseek, "lseek" } },
    { ProxyKernelServer::SYS_brk, { &ProxyKernelServer::sys_brk, "brk" } },
    { ProxyKernelServer::SYS_linkat, { &ProxyKernelServer::sys_linkat, "linkat" } },
    { ProxyKernelServer::SYS_unlinkat, { &ProxyKernelServer::sys_unlinkat, "unlinkat" } },
    { ProxyKernelServer::SYS_mkdirat, { &ProxyKernelServer::sys_mkdirat, "mkdirat" } },
    { ProxyKernelServer::SYS_renameat, { &ProxyKernelServer::sys_renameat, "renameat" } },
    { ProxyKernelServer::SYS_chdir, { &ProxyKernelServer::sys_chdir, "chdir" } },
    { ProxyKernelServer::SYS_getcwd, { &ProxyKernelServer::sys_getcwd, "getcwd" } },
    { ProxyKernelServer::SYS_fstat, { &ProxyKernelServer::sys_fstat, "fstat" } },
    { ProxyKernelServer::SYS_fstatat, { &ProxyKernelServer::sys_fstatat, "fstatat" } },
    { ProxyKernelServer::SYS_faccessat, { &ProxyKernelServer::sys_faccessat, "faccessat" } },
    { ProxyKernelServer::SYS_pread, { &ProxyKernelServer::sys_pread, "pread" } },
    { ProxyKernelServer::SYS_pwrite, { &ProxyKernelServer::sys_pwrite, "pwrite" } },
    { ProxyKernelServer::SYS_uname, { &ProxyKernelServer::sys_uname, "uname" } },
    { ProxyKernelServer::SYS_getuid, { &ProxyKernelServer::sys_getuid, "getuid" } },
    { ProxyKernelServer::SYS_geteuid, { &ProxyKernelServer::sys_geteuid, "geteuid" } },
    { ProxyKernelServer::SYS_getgid, { &ProxyKernelServer::sys_getgid, "getgid" } },
    { ProxyKernelServer::SYS_getegid, { &ProxyKernelServer::sys_getegid, "getegid" } },
    { ProxyKernelServer::SYS_mmap, { &ProxyKernelServer::sys_mmap, "mmap" } },
    { ProxyKernelServer::SYS_munmap, { &ProxyKernelServer::sys_munmap, "munmap" } },
    { ProxyKernelServer::SYS_mremap, { &ProxyKernelServer::sys_mremap, "mremap" } },
    { ProxyKernelServer::SYS_mprotect, { &ProxyKernelServer::sys_mprotect, "mprotect" } },
    { ProxyKernelServer::SYS_prlimit64, { &ProxyKernelServer::sys_prlimit64, "prlimit64" } },
    { ProxyKernelServer::SYS_getmainvars, { &ProxyKernelServer::sys_getmainvars, "getmainvars" } },
    { ProxyKernelServer::SYS_rt_sigaction, { &ProxyKernelServer::sys_rt_sigaction, "rt_sigaction" } },
    { ProxyKernelServer::SYS_writev, { &ProxyKernelServer::sys_writev, "writev" } },
    { ProxyKernelServer::SYS_gettimeofday, { &ProxyKernelServer::sys_gettimeofday, "gettimeofday" } },
    { ProxyKernelServer::SYS_times, { &ProxyKernelServer::sys_times, "times" } },
    { ProxyKernelServer::SYS_fcntl, { &ProxyKernelServer::sys_fcntl, "fcntl" } },
    { ProxyKernelServer::SYS_ftruncate, { &ProxyKernelServer::sys_ftruncate, "ftruncate" } },
    { ProxyKernelServer::SYS_getdents, { &ProxyKernelServer::sys_getdents, "getdents" } },
    { ProxyKernelServer::SYS_dup, { &ProxyKernelServer::sys_dup, "dup" } },
    { ProxyKernelServer::SYS_dup3, { &ProxyKernelServer::sys_dup3, "dup3" } },
    { ProxyKernelServer::SYS_readlinkat, { &ProxyKernelServer::sys_readlinkat, "readlinkat" } },
    { ProxyKernelServer::SYS_rt_sigprocmask, { &ProxyKernelServer::sys_rt_sigprocmask, "rt_sigprocmask" } },
    { ProxyKernelServer::SYS_ioctl, { &ProxyKernelServer::sys_ioctl, "ioctl" } },
    { ProxyKernelServer::SYS_getrlimit, { &ProxyKernelServer::sys_getrlimit, "getrlimit" } },
    { ProxyKernelServer::SYS_setrlimit, { &ProxyKernelServer::sys_setrlimit, "setrlimit" } },
    { ProxyKernelServer::SYS_getrusage, { &ProxyKernelServer::sys_getrusage, "getrusage" } },
    { ProxyKernelServer::SYS_clock_gettime, { &ProxyKernelServer::sys_clock_gettime, "clock_gettime" } },
    { ProxyKernelServer::SYS_set_tid_address, { &ProxyKernelServer::sys_set_tid_address, "set_tid_address" } },
    { ProxyKernelServer::SYS_set_robust_list, { &ProxyKernelServer::sys_set_robust_list, "set_robust_list" } },
    { ProxyKernelServer::SYS_madvise, { &ProxyKernelServer::sys_madvise, "madvise" } },
    { ProxyKernelServer::SYS_open, { &ProxyKernelServer::sys_open, "open" } },
    { ProxyKernelServer::SYS_link, { &ProxyKernelServer::sys_link, "link" } },
    { ProxyKernelServer::SYS_unlink, { &ProxyKernelServer::sys_unlink, "unlink" } },
    { ProxyKernelServer::SYS_mkdir, { &ProxyKernelServer::sys_mkdir, "mkdir" } },
    { ProxyKernelServer::SYS_access, { &ProxyKernelServer::sys_access, "access" } },
    { ProxyKernelServer::SYS_stat, { &ProxyKernelServer::sys_stat, "stat" } },
    { ProxyKernelServer::SYS_lstat, { &ProxyKernelServer::sys_lstat, "lstat" } },
    { ProxyKernelServer::SYS_time, { &ProxyKernelServer::sys_time, "time" } }
};

__uint64_t ProxyKernelServer::sys_exit(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    events->EnqueueEvent(shutdownEvent);
    return 0;
}

__uint64_t ProxyKernelServer::sys_exit_group(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_getpid(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_kill(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_read(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_write(__uint64_t fd, __uint64_t addr, __uint64_t size, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    std::ostream *fs = ostreamFromFD(fd);
    if (fs == nullptr)
        return -1; // TODO errno
    char *buf = new char[size];
    bus->Read64(addr, size, buf);
    fs->write(buf, size);
    delete[] buf;
    return size;
}

__uint64_t ProxyKernelServer::sys_openat(__uint64_t dirfd, __uint64_t fn, __uint64_t fn_size, __uint64_t flags, __uint64_t mode, __uint64_t arg5, __uint64_t arg6) {

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

__uint64_t ProxyKernelServer::sys_close(__uint64_t fd, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
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

__uint64_t ProxyKernelServer::sys_lseek(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_brk(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_linkat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_unlinkat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_mkdirat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_renameat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_chdir(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_getcwd(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_fstat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_fstatat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_faccessat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_pread(__uint64_t fd, __uint64_t addr, __uint64_t size,
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

__uint64_t ProxyKernelServer::sys_pwrite(__uint64_t fd, __uint64_t addr, __uint64_t size,
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

__uint64_t ProxyKernelServer::sys_uname(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_getuid(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_geteuid(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_getgid(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_getegid(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_mmap(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_munmap(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_mremap(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_mprotect(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_prlimit64(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_getmainvars(__uint64_t addr, __uint64_t size, __uint64_t arg2, __uint64_t arg3,  __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
 
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

__uint64_t ProxyKernelServer::sys_rt_sigaction(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_writev(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_gettimeofday(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_times(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_fcntl(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_ftruncate(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_getdents(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_dup(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_dup3(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_readlinkat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_rt_sigprocmask(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_ioctl(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_getrlimit(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_setrlimit(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_getrusage(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_clock_gettime(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_set_tid_address(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_set_robust_list(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_madvise(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_open(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_link(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_unlink(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_mkdir(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_access(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_stat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_lstat(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

__uint64_t ProxyKernelServer::sys_time(__uint64_t arg0, __uint64_t arg1, __uint64_t arg2, __uint64_t arg3, __uint64_t arg4, __uint64_t arg5, __uint64_t arg6) {
    return -ProxyKernelServer::ERR_enosys;
}

} // namespace CASK
