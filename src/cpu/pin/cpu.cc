#include "cpu/pin/cpu.hh"

#include <cstdlib>
#include <fcntl.h>
#include <sys/wait.h>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <sys/times.h>
#include <sys/mman.h>

#include "cpu/simple_thread.hh"
#include "params/BasePinCPU.hh"
#include "cpu/pin/message.hh"
#include "debug/Pin.hh"
#include "sim/system.hh"
#include "arch/x86/regs/int.hh"
#include "arch/x86/cpuid.hh"
#include "arch/x86/isa.hh"
#include "sim/faults.hh"
#include "arch/x86/utility.hh"
#include "cpu/pin/regfile.h"
#include "base/loader/symtab.hh"
#include "base/output.hh"
#include "sim/sim_exit.hh"
#include "arch/x86/utility.hh"

#define HFI_CODE_RANGE(X, i)                    \
    X(metadata.code_ranges[i - 1].executable, LINEAR_CODERANGE_##i##_EXECUTABLE) \
    X(metadata.code_ranges[i - 1].base_mask, LINEAR_CODERANGE_##i##_BASE_MASK) \
    X(metadata.code_ranges[i - 1].ignore_mask, LINEAR_CODERANGE_##i##_IGNORE_MASK)
    
#define HFI_CODE_RANGES(X)                      \
    HFI_CODE_RANGE(X, 1)                        \
    HFI_CODE_RANGE(X, 2)
    
#define HFI_DATA_RANGE(X, i)                    \
    X(metadata.data_ranges[i - 1].readable, LINEAR_RANGE_##i##_READABLE) \
    X(metadata.data_ranges[i - 1].writeable, LINEAR_RANGE_##i##_WRITEABLE) \
    X(metadata.data_ranges[i - 1].rangesizetype, LINEAR_RANGE_##i##_RANGESIZETYPE) \
    X(metadata.data_ranges[i - 1].base_address_base_mask, LINEAR_RANGE_##i##_BASE_ADDRESS_BASE_MASK) \
    X(metadata.data_ranges[i - 1].offset_limit_ignore_mask, LINEAR_RANGE_##i##_OFFSET_LIMIT_IGNORE_MASK)
    
#define HFI_DATA_RANGES(X)                       \
    HFI_DATA_RANGE(X, 1)                         \
    HFI_DATA_RANGE(X, 2)                         \
    HFI_DATA_RANGE(X, 3)                         \
    HFI_DATA_RANGE(X, 4)
        
#define HFI_STATE(X)                             \
    X(metadata.is_trusted_sandbox, IS_TRUSTED_SANDBOX)    \
    X(inside_sandbox, INSIDE_SANDBOX)       \
    X(metadata.exit_sandbox_handler, EXIT_SANDBOX_HANDLER)   \
    X(exit_reason, EXIT_REASON)                              \
    X(exit_location, EXIT_LOCATION)                          \
    HFI_DATA_RANGES(X)                              \
    HFI_CODE_RANGES(X)

namespace gem5
{

namespace pin
{

static std::vector<std::string_view>
split_by_spaces(std::string_view str)
{
    std::vector<std::string_view> result;
    size_t pos = 0;
    size_t size = str.size();

    while (pos < size) {
        // Skip any leading spaces
        while (pos < size && std::isspace(static_cast<unsigned char>(str[pos]))) {
            ++pos;
        }

        if (pos >= size) {
            break;
        }

        // Find the end of the current word
        size_t start = pos;
        while (pos < size && !std::isspace(static_cast<unsigned char>(str[pos]))) {
            ++pos;
        }

        // Create a string_view for the current word
        result.emplace_back(str.substr(start, pos - start));
    }

    return result;
}

CPU::CPU(const BasePinCPUParams &params)
    : BaseCPU(params),
      tickEvent([this] { tick(); }, "BasePinCPU tick", false, Event::CPU_Tick_Pri),
      _status(Idle),
      dataPort(name() + ".dcache_port", this),
      instPort(name() + ".icache_port", this),
      pinExe(params.pinExe),
      pinKernel(params.pinKernel),
      pinTool(params.pinTool),
      pinPid(-1),
      system(params.system),
      traceInsts(params.traceInsts),
      enableBBV(params.enableBBV),
      interval(params.interval)
{
    thread = std::make_unique<SimpleThread>(
        this, /*thread_num*/0, params.system,
        params.workload[0], params.mmu,
        params.isa[0], params.decoder[0]);
    thread->setStatus(ThreadContext::Halted);
    tc = thread->getTC();
    threadContexts.push_back(tc);

    if (params.countInsts)
        ctrInsts = 0;

    // Parse PinTool arguments.
    for (std::string_view sv : split_by_spaces(params.pinArgs))
        pinArgs.emplace_back(sv);
    for (std::string_view sv : split_by_spaces(params.pinToolArgs))
        pinToolArgs.emplace_back(sv);
}

void
CPU::haltContext()
{
    DPRINTF(Pin, "Halting Pin process\n");
    // Tell Pin to exit.
    assert(pinPid >= 0 && reqFd >= 0 && respFd >= 0);

    Message msg;
    msg.type = Message::Exit;
    msg.send(reqFd);
    
    close(reqFd);
    close(respFd);

    if (waitpid(pinPid, nullptr, 0) < 0)
        panic("waitpid failed!\n");

    // Dump times.
    struct tms tms;
    if (times(&tms) < 0)
        panic("times(2) failed\n");
    const auto tick = sysconf(_SC_CLK_TCK);
    DPRINTF(Pin, "user.pin: %fs, sys.pin: %fs, user.gem5: %fs, sys.gem5: %fs\n", 
            static_cast<double>(tms.tms_cutime) / tick,
            static_cast<double>(tms.tms_cstime) / tick,
	    static_cast<double>(tms.tms_utime) / tick,
	    static_cast<double>(tms.tms_stime) / tick);

    _status = Idle;
}

bool
CPU::PinRequestPort::recvTimingResp(PacketPtr pkt)
{
    fatal("Unsupported: %s", __func__);
}

void
CPU::PinRequestPort::recvReqRetry()
{
    fatal("Unsupported: %s", __func__);
}

Port &
CPU::getDataPort()
{
    return dataPort;
}

Port &
CPU::getInstPort()
{
    return instPort;
}

void
CPU::wakeup(ThreadID tid)
{
    fatal("Unsupported: %s", __func__);
}

Counter
CPU::totalInsts() const
{
    return ctrInsts ? *ctrInsts : 0;
}

Counter
CPU::totalOps() const
{
    warn_once("Pretending totalInsts == totalOps\n");
    return totalInsts();
}

CPU *getCPU(const BasePinCPUParams &params) {
    return new CPU(params);
}

const char *
CPU::getPinRoot()
{
    const char *pin_root = std::getenv("PIN_ROOT");
    fatal_if(pin_root == nullptr, "environment variable PIN_ROOT not set!"); // TODO: Move this to build variable?
    return pin_root;
}

const std::string&
CPU::getPinExe() const
{
    return pinExe;
}

const std::string&
CPU::getPinTool() const
{
    return pinTool;
}

const char *
CPU::getRequestPath()
{
    const char *req_path = std::getenv("PIN_REQ");
    fatal_if(req_path == nullptr, "environment variable PIN_REQ not set!");
    return req_path;
}

const char *
CPU::getResponsePath()
{
    const char *resp_path = std::getenv("PIN_RESP");
    fatal_if(resp_path == nullptr, "environment variable PIN_RESP not set!");
    return resp_path;
}

const std::string&
CPU::getDummyProg() const
{
    return pinKernel;
}

void
CPU::init()
{
    BaseCPU::init();
    fatal_if(numThreads != 1, "Pin: Multithreading not supported");
    kernoutPath = tc->getProcessPtr()->absolutePath(simout.resolve("kernout.txt"), true);
    kernerrPath = tc->getProcessPtr()->absolutePath(simout.resolve("kernerr.txt"), true);
    warn("Pin::CPU::init not complete\n");
}

void
CPU::startup()
{
    BaseCPU::startup();

    // TODO: Remove this crap. unused i think.
    tc->simcall_info.type = ThreadContext::SimcallInfo::INVALID; // TODO: This is definitely not the appropriate place for this.

    // Create pipes for bidirectional communication.
    // TODO: Wrap these as C FILEs, so that we don't have to worry about partial read(2)'s and write(2)'s.
    int req_fds[2];
    if (pipe(req_fds) < 0)
        fatal("pipe failed: %s", std::strerror(errno));
    int resp_fds[2];
    if (pipe(resp_fds) < 0)
        fatal("pipe failed: %s", std::strerror(errno));
    reqFd = req_fds[1];
    respFd = resp_fds[0];
    const int remote_req_fd = req_fds[0];
    const int remote_resp_fd = resp_fds[1];

    // TODO: Pass fd's directly to Pintool?
    char req_path[32];
    std::sprintf(req_path, "/dev/fd/%d", remote_req_fd);
    char resp_path[32];
    std::sprintf(resp_path, "/dev/fd/%d", remote_resp_fd);
    const std::string pin_tool = getPinTool();
    const std::string pin_exe = getPinExe();
    const std::string dummy_prog = getDummyProg();

    std::stringstream shm_path_ss;
    const auto &backing_store = system->getPhysMem().getBackingStore();
    fatal_if(backing_store.size() != 1, "Pin CPU supports only one backing store entry");
    const int shm_fd = backing_store[0].shmFd;
    fatal_if(shm_fd < 0, "Pin CPU requires shared memory backing store");
    shm_path_ss << "/dev/fd/" << shm_fd;
    const std::string shm_path = shm_path_ss.str();

    int shm_fd_flags;
    if ((shm_fd_flags = fcntl(shm_fd, F_GETFD)) < 0)
        fatal("fcntl FD_GETFD failed");
    shm_fd_flags &= ~FD_CLOEXEC;
    if (fcntl(shm_fd, F_SETFD, shm_fd_flags) < 0)
        fatal("fcntl FD_SETFD failed");

    pinPid = fork();
    if (pinPid < 0) {
        fatal("fork: %s", std::strerror(errno));
    } else if (pinPid == 0) {
        // Create log file for this fucking mess.
        // It will be for the kernel.
        const int kernout_fd = open(kernoutPath.c_str(), O_WRONLY | O_APPEND | O_TRUNC | O_CREAT, 0664);
        if (kernout_fd < 0)
            panic("Failed to create kernel.log\n");
        if (dup2(kernout_fd, STDOUT_FILENO) < 0)
            panic("dup2 failed\n");

        const int kernerr_fd = open(kernerrPath.c_str(), O_WRONLY | O_APPEND | O_TRUNC | O_CREAT, 0664);
        if (kernerr_fd < 0)
            panic("Failed to create kernerr.txt");
        if (dup2(kernerr_fd, STDERR_FILENO) < 0)
            panic("dup2 failed\n");

        
        // This is the Pin subprocess. Execute pin.
        std::vector<std::string> args;
        auto it = std::back_inserter(args);

        // Pin executable.
        *it++ = pin_exe;

        // Pin args.
        if (std::getenv("PIN_APPDEBUG")) {
            *it++ = "-appdebug"; *it++ = "1";
        }
        if (std::getenv("PIN_TOOLDEBUG")) {
            *it++ = "-pause_tool"; *it++ = "30";
        }

        it = std::copy(pinArgs.begin(), pinArgs.end(), it);

        // Pintool.
        *it++ = "-t"; *it++ = pin_tool;

        // Pintool args.
        *it++ = "-log"; *it++ = simout.resolve("pin.log");
        *it++ = "-req_path"; *it++ = req_path;
        *it++ = "-resp_path"; *it++ = resp_path;
        *it++ = "-mem_path"; *it++ = shm_path;
        *it++ = "-instcount"; *it++ = ctrInsts ? "1" : "0";

        // Custom Pintool args.
        it = std::copy(pinToolArgs.begin(), pinToolArgs.end(), it);

        // Workload.
        *it++ = "--";
        *it++ = dummy_prog;

        std::vector<char *> args_c;
        for (const std::string &s : args)
            args_c.push_back(const_cast<char *>(s.c_str()));
        args_c.push_back(nullptr);

        std::stringstream cmd_ss;
        for (const std::string &arg : args)
            cmd_ss << arg << " ";
        const std::string cmd_s = cmd_ss.str();
        dprintf(kernout_fd, "Starting Pin: %s\n", cmd_s.c_str());
        
        execvp(args_c[0], args_c.data());
        fatal("execvp failed: %s: %s", args_c[0], std::strerror(errno));
    }

    // Close the remote end of the socket; it will remain open in the Pin subprocess.
    close(req_fds[0]); // Close read-end of request pipe.
    close(resp_fds[1]); // Close write-end of response pipe.
    
    // Send initial ACK.
    Message msg;
    msg.type = Message::Ack;
    DPRINTF(Pin, "Sending initial ACK\n");
    msg.send(reqFd);
    DPRINTF(Pin, "Receiving initial ACK\n");
    msg.recv(respFd);
    panic_if(msg.type != Message::Ack, "Received message other than ACK at pintool startup!\n");
    DPRINTF(Pin, "received ACK from pintool\n");

    // Copy over initial state.
    syncStateToPin(true);

    // Map in code.
    mapCode();
}

void
CPU::mapCode()
{
    const Addr min = tc->getProcessPtr()->image.minAddr();
    const Addr max = tc->getProcessPtr()->image.maxAddr();
    const TranslationGenPtr ptr = tc->getMMUPtr()->translateFunctional(min, max - min, tc, BaseMMU::Execute, 0);
    for (TranslationGenConstIterator it = ptr->begin(); it != ptr->end(); ++it) {
        const TranslationGen::Range &range = *it;
        if (range.fault != NoFault) {
            it.resetFault();
            continue;
        }
        DPRINTF(Pin, "Mapping in code range vaddr=%#x paddr=%#x size=%#x\n",
                range.vaddr, range.paddr, range.size);
        Message msg;
        msg.type = Message::Map;
        msg.map.vaddr = range.vaddr;
        msg.map.paddr = range.paddr;
        msg.map.size = range.size;
        msg.map.prot = PROT_READ | PROT_EXEC;
        msg.send(reqFd);
        msg.recv(respFd);
        panic_if(msg.type != Message::Ack, "unexpected response\n");
    }
}

void
CPU::activateContext(ThreadID tid)
{
    assert(tid == 0);
    assert(thread);

    schedule(tickEvent, clockEdge(Cycles(0)));
    _status = Running;
}

void
CPU::suspendContext(ThreadID tid)
{
    assert(tid == 0);
    assert(thread);

    if (_status == Idle)
        return;

    assert(_status == Running);

    if (tickEvent.scheduled())
        deschedule(tickEvent);

    _status = Idle;
}

void
CPU::tick()
{
    Tick delay = 0;

    assert(_status != Idle);
    assert(_status == Running);

    pinRun(); // TODO: Will need to communicate how many ticks required.

    if (tc->status() == ThreadContext::Halting ||
        tc->status() == ThreadContext::Halted) {
        haltContext();
    }
    
    ++delay; // FIXME

    if (_status != Idle) {
        schedule(tickEvent, clockEdge(ticksToCycles(delay)));
    }
}

  // TODO: Can rewrite this without using macros. 
#define FOREACH_IREG() \
    do { \
        APPLY_IREG(rax, X86ISA::int_reg::Rax); \
        APPLY_IREG(rbx, X86ISA::int_reg::Rbx); \
        APPLY_IREG(rcx, X86ISA::int_reg::Rcx); \
        APPLY_IREG(rdx, X86ISA::int_reg::Rdx); \
        APPLY_IREG(rsi, X86ISA::int_reg::Rsi); \
        APPLY_IREG(rdi, X86ISA::int_reg::Rdi); \
        APPLY_IREG(rsp, X86ISA::int_reg::Rsp); \
        APPLY_IREG(rbp, X86ISA::int_reg::Rbp); \
        APPLY_IREG(r8,  X86ISA::int_reg::R8); \
        APPLY_IREG(r9,  X86ISA::int_reg::R9); \
        APPLY_IREG(r10, X86ISA::int_reg::R10); \
        APPLY_IREG(r11, X86ISA::int_reg::R11); \
        APPLY_IREG(r12, X86ISA::int_reg::R12); \
        APPLY_IREG(r13, X86ISA::int_reg::R13); \
        APPLY_IREG(r14, X86ISA::int_reg::R14); \
        APPLY_IREG(r15, X86ISA::int_reg::R15); \
    } while (0)

void
CPU::syncRegvalToPin(const char *regname, const void *data, size_t size)
{
    // Construct message.
    Message msg;
    msg.type = Message::SetReg;
    std::snprintf(msg.reg.name, sizeof msg.reg.name, "%s", regname);
    assert(size < sizeof msg.reg.data);
    std::memcpy(msg.reg.data, data, size);
    msg.reg.size = size;

    // Send and receive.
    DPRINTF(Pin, "Sending SET_REG for %s\n", regname);
    msg.send(reqFd);
    msg.recv(respFd);
    panic_if(msg.type != Message::Ack, "received response other than ACK (%i): %s!\n", msg.type, msg);    
}

template <typename T>
void
CPU::syncRegvalToPin(const char *regname, T value)
{
    syncRegvalToPin(regname, &value, sizeof value);
}

void
CPU::syncSingleRegToPin(const char *regname, const RegId &reg)
{
    // Read register value.
    std::vector<uint8_t> data(reg.regClass().regBytes());
    tc->getReg(reg, data.data());

    syncRegvalToPin(regname, data.data(), data.size());
}

void
CPU::syncStateToPin(bool full)
{
    using namespace X86ISA;
    Message msg;
    msg.type = Message::SetRegs;
    PinRegFile &rf = msg.regfile;

    // Set integer registers.
    rf.rax = tc->getReg(int_reg::Rax);
    rf.rbx = tc->getReg(int_reg::Rbx);
    rf.rcx = tc->getReg(int_reg::Rcx);
    rf.rdx = tc->getReg(int_reg::Rdx);
    rf.rdi = tc->getReg(int_reg::Rdi);
    rf.rsi = tc->getReg(int_reg::Rsi);
    rf.rbp = tc->getReg(int_reg::Rbp);
    rf.rsp = tc->getReg(int_reg::Rsp);
    rf.r8 = tc->getReg(int_reg::R8);
    rf.r9 = tc->getReg(int_reg::R9);
    rf.r10 = tc->getReg(int_reg::R10);
    rf.r11 = tc->getReg(int_reg::R11);
    rf.r12 = tc->getReg(int_reg::R12);
    rf.r13 = tc->getReg(int_reg::R13);
    rf.r14 = tc->getReg(int_reg::R14);
    rf.r15 = tc->getReg(int_reg::R15);
    rf.rip = tc->pcState().instAddr();

    // Set floating-point registers.
    for (int i = 0; i < 8; ++i) {
        const double value64 = bitsToFloat64(tc->getReg(float_reg::fpr(i)));
        storeFloat80(&rf.fprs[i][0], value64);
    }
    for (int i = 0; i < 16; ++i) {
        rf.xmms[i][0] = tc->getReg(float_reg::xmmLow(i));
        rf.xmms[i][1] = tc->getReg(float_reg::xmmHigh(i));
    }
    rf.fcw = tc->readMiscRegNoEffect(misc_reg::Fcw);
    rf.fsw = tc->readMiscRegNoEffect(misc_reg::Fsw);
    rf.ftag = tc->readMiscRegNoEffect(misc_reg::Ftag);

    // Misc registers.
    rf.rflags = getRFlags(tc);
    rf.fs = tc->readMiscRegNoEffect(misc_reg::Fs);
    rf.gs = tc->readMiscRegNoEffect(misc_reg::Gs);
    rf.fs_base = tc->readMiscRegNoEffect(misc_reg::FsBase);
    rf.gs_base = tc->readMiscRegNoEffect(misc_reg::GsBase);

    // HFI registers.
#define X(rf_name, misc_name) rf.hfi.rf_name = tc->readMiscRegNoEffect(misc_reg::HFI_##misc_name);
    HFI_STATE(X)
#undef X

    // Send message.
    msg.send(reqFd);
    msg.recv(respFd);
    panic_if(msg.type != Message::Ack, "Got message other than ACK for SetRegs!\n");
}

void
CPU::syncRegvalFromPin(const char *regname, void *data, size_t size)
{
    // Construct message.
    Message msg;
    msg.type = Message::GetReg;
    std::snprintf(msg.reg.name, sizeof msg.reg.name, "%s", regname);
    msg.reg.size = size;

    // Send and receive.
    DPRINTF(Pin, "Sending GET_REG for %s\n", regname);
    msg.send(reqFd);
    msg.recv(respFd);
    panic_if(msg.type != Message::SetReg, "received response other than SET_REG (%i): %s\n", msg.type, msg);

    // Set register.
    panic_if(msg.reg.size != size, "Got bad register size\n");
    panic_if(std::strncmp(msg.reg.name, regname, sizeof msg.reg.name) != 0, "Got bad register name\n");
    std::memcpy(data, msg.reg.data, size);
}

template <typename T>
T
CPU::syncRegvalFromPin(const char *name)
{
    T value;
    syncRegvalFromPin(name, &value, sizeof value);
    return value;
}

void
CPU::syncRegFromPin(const char *regname, const RegId &reg)
{
    std::vector<uint8_t> buf(reg.regClass().regBytes());
    syncRegvalFromPin(regname, buf.data(), buf.size());

    if (buf.size() == 8)
        DPRINTF(Pin, "GET_REG: %s %x\n", regname, * (const uint64_t *) buf.data());
    
    tc->setReg(reg, buf.data());
}

void
CPU::syncStateFromPin(bool full)
{
    using namespace X86ISA;
    
    Message msg;
    msg.type = Message::GetRegs;
    msg.send(reqFd);
    msg.recv(respFd);
    panic_if(msg.type != Message::SetRegs, "Got response other than SetRegs in response to GetRegs!\n");
    const PinRegFile &rf = msg.regfile;

    // Copy all integer registers.
    tc->setReg(int_reg::Rax, rf.rax);
    tc->setReg(int_reg::Rbx, rf.rbx);
    tc->setReg(int_reg::Rcx, rf.rcx);
    tc->setReg(int_reg::Rdx, rf.rdx);
    tc->setReg(int_reg::Rdi, rf.rdi);
    tc->setReg(int_reg::Rsi, rf.rsi);
    tc->setReg(int_reg::Rbp, rf.rbp);
    tc->setReg(int_reg::Rsp, rf.rsp);
    tc->setReg(int_reg::R8, rf.r8);
    tc->setReg(int_reg::R9, rf.r9);
    tc->setReg(int_reg::R10, rf.r10);
    tc->setReg(int_reg::R11, rf.r11);
    tc->setReg(int_reg::R12, rf.r12);
    tc->setReg(int_reg::R13, rf.r13);
    tc->setReg(int_reg::R14, rf.r14);
    tc->setReg(int_reg::R15, rf.r15);
    tc->pcState(rf.rip);

    // Floating-point registers.
    for (int i = 0; i < 8; ++i) {
        const double value64 = loadFloat80(rf.fprs[i]);
        const uint64_t bits64 = floatToBits64(value64);
        tc->setReg(float_reg::fpr(i), bits64);
    }
    for (int i = 0; i < 16; ++i) {
        tc->setReg(float_reg::xmmLow(i), rf.xmms[i][0]);
        tc->setReg(float_reg::xmmHigh(i), rf.xmms[i][1]);
    }
    tc->setMiscRegNoEffect(misc_reg::Fcw, rf.fcw);
    tc->setMiscRegNoEffect(misc_reg::Fsw, rf.fsw);
    tc->setMiscRegNoEffect(misc_reg::Ftag, rf.ftag);
    tc->setMiscRegNoEffect(misc_reg::Ftw, rf.ftag); // TODO: Not sure if this is right, but it's what KVM does.
    
    // Misc registers.
    setRFlags(tc, rf.rflags);
    tc->setMiscRegNoEffect(misc_reg::Fs, rf.fs);
    tc->setMiscRegNoEffect(misc_reg::Gs, rf.gs);
    tc->setMiscRegNoEffect(misc_reg::FsBase, rf.fs_base);
    tc->setMiscRegNoEffect(misc_reg::GsBase, rf.gs_base);

    // HFI registers.
#define X(rf_name, misc_name) tc->setMiscRegNoEffect(misc_reg::HFI_##misc_name, rf.hfi.rf_name);
    HFI_STATE(X)
#undef X
}

void
CPU::pinRun()
{
    syncStateToPin(false);

    // Tell it to run.
    Message msg;
    msg.type = Message::Run;
    msg.send(reqFd);
    msg.recv(respFd);
    if (ctrInsts) {
        const std::string instcount_s = executePinCommand("instcount");
        const auto new_instcount = std::stoull(instcount_s);
        assert(*ctrInsts <= new_instcount);
        ctrInsts = new_instcount;
    }

    switch (msg.type) {
      case Message::PageFault:
        handlePageFault(msg.faultaddr);
        break;

      case Message::Syscall:
        handleSyscall();
        break;

      case Message::Cpuid:
        handleCPUID();
        break;

      case Message::Break:
        syncStateFromPin(false);
        exitSimLoopNow("pin-breakpoint");
        break;

      case Message::Ack:
        break;
        
      default:
        panic("unhandled run response type (%d)\n", msg.type);
    }
}

void
CPU::handlePageFault(Addr vaddr)
{
    syncStateFromPin(false);
  
    DPRINTF(Pin, "vaddr=%x\n", vaddr);
    assert(vaddr);
    vaddr &= ~ (Addr) 0xfff;

    // New approach:
    // Just start grabbing mappings until they aren't mergeable.
    struct Entry {
        Addr vaddr;
        Addr paddr;
        size_t size;
        int prot;
    };
    std::list<Entry> mappings;
    MemState& mem_state = *tc->getProcessPtr()->memState;
    size_t size = 0x1000;
    if (VMA *vma = mem_state.getVMA(vaddr)) {
        vaddr = vma->start();
        size = vma->size();
        DPRINTF(Pin, "VMA: %#x %#x %s\n", vma->start(), vma->end(), vma->getName());
    }
    DPRINTF(Pin, "Preparing to map starting at vaddr=%#x size=%#x\n",
            vaddr, size);

#if 1
    // Collect list of page mappings with permissions.
    const auto translate_with_mode = [&] (BaseMMU::Mode mode) -> TranslationGenPtr {
        return tc->getMMUPtr()->translateFunctional(vaddr, size, tc, mode, 0);
    };
    const auto r = translate_with_mode(BaseMMU::Read);
    const auto w = translate_with_mode(BaseMMU::Write);
    const auto x = translate_with_mode(BaseMMU::Execute);
    for (auto r_it = r->begin(), w_it = w->begin(), x_it = x->begin();
         r_it != r->end();
         ++r_it, ++w_it, ++x_it) {
        assert(w_it != w->end());
        assert(x_it != x->end());
        panic_if(r_it->fault != NoFault, "Page fault: vaddr=%#x fault=%s\n", r_it->vaddr, r_it->fault->name());
        Entry entry;
        entry.vaddr = r_it->vaddr;
        entry.paddr = r_it->paddr;
        entry.size = r_it->size;
        entry.prot = PROT_READ;
        if (w_it->fault == NoFault)
            entry.prot |= PROT_WRITE;
        if (x_it->fault == NoFault)
            entry.prot |= PROT_EXEC;
        mappings.push_back(entry);
    }
#else
    const auto ptr = tc->getMMUPtr()->translateFunctional(vaddr, size, tc, BaseMMU::Read, 0);
    const auto exec_ptr = tc->getMMUPtr()->translateFunctional(vaddr, size, tc, BaseMMU::Execute, 0);
    for (const TranslationGen::Range& range : *ptr) {
        panic_if(range.fault != NoFault, "Page fault: vaddr=%#x fault=%s\n", range.vaddr, range.fault->name());
        Entry entry;
        entry.vaddr = range.vaddr;
        entry.paddr = range.paddr;
        entry.size = range.size;
        mappings.push_back(entry);
    }

#endif

    // Combine into ranges.
    assert(!mappings.empty());
    for (auto it1 = mappings.begin(); std::next(it1) != mappings.end(); ) {
        const auto it2 = std::next(it1);

        // Can we combine the entries pointed to by it1 and it2?
        Entry& e1 = *it1;
        Entry& e2 = *it2;
        assert(e1.vaddr + e1.size == e2.vaddr);
        if (e1.prot == e2.prot && e1.paddr + e1.size == e2.paddr) {
            // Yes, can combine!
            e1.size += e2.size;
            mappings.erase(it2);
        } else {
            // No, we can't combine, so advance.
            ++it1;
        }
    }

    // Send ranges over.
    for (const Entry &e : mappings) {
        DPRINTF(Pin, "Mapping vaddr=%#x paddr=%#x size=%#x prot=%#x\n",
                e.vaddr, e.paddr, e.size, e.prot);
        Message msg;
        msg.type = Message::Map;
        msg.map.vaddr = e.vaddr;
        msg.map.paddr = e.paddr;
        msg.map.size = e.size;
        msg.map.prot = e.prot;
        msg.send(reqFd);
        msg.recv(respFd);
        panic_if(msg.type != Message::Ack, "unexpected response\n");        
    }
}

Tick
CPU::doMMIOAccess(Addr paddr, void *data, int size, bool write)
{
    // TODO: Remove this entirely.
    fatal("delete this bloody function\n");
    
    // NOTE: Might need to stutterPC like in KVM:
    // pc.as<X86ISA::PCState>().setNPC(pc.instAddr()); 
    syncStateFromPin(false);

    RequestPtr mmio_req = std::make_shared<Request>(
        paddr, size, Request::UNCACHEABLE, dataRequestorId());

    mmio_req->setContext(tc->contextId());

    // Skip finalization of MMIO physical address.

    const MemCmd cmd(write ? MemCmd::WriteReq : MemCmd::ReadReq);
    PacketPtr pkt = new Packet(mmio_req, cmd);
    pkt->dataStatic(data);

    warn_if(!mmio_req->isLocalAccess(), "MMIO request is not local access. I have no clue what this means.\n");

    const Cycles ipr_delay = mmio_req->localAccessor(tc, pkt);
    // threadContextDirty = true;
    delete pkt;
    return clockPeriod() * ipr_delay;
}

void
CPU::handleSyscall()
{
    syncStateFromPin(false);

    tc->getSystemPtr()->workload->syscall(tc);

    // If we unmapped any pages, then tell pin that here.
    auto& unmapped = tc->getProcessPtr()->memState->unmapped;
    auto unmapped_it = unmapped.begin();
    while (unmapped_it != unmapped.end()) {
        const Addr vbase = *unmapped_it;
        Addr vsize = 0x1000;
        for (++unmapped_it;
             unmapped_it != unmapped.end() && *unmapped_it == vbase + vsize;
             ++unmapped_it, vsize += 0x1000)
            ;
        DPRINTF(Pin, "Pin: unmapping vaddr %#x-%#x\n", vbase, vbase + vsize);
        Message msg;
        msg.type = Message::Unmap;
        msg.map.vaddr = vbase;
        msg.map.size = vsize;
        msg.send(reqFd);
        msg.recv(respFd);
        panic_if(msg.type != Message::Ack, "unexpected response\n");
    }
    unmapped.clear();

    // FIXME: Need to cleanly exit. 
}

void
CPU::handleCPUID()
{
    syncStateFromPin(false);

    // Get function (EAX).
    const uint32_t func = tc->getReg(X86ISA::int_reg::Rax);

    // Get index (ECX).
    const uint32_t index = tc->getReg(X86ISA::int_reg::Rcx);

    DPRINTF(Pin, "CPUID: EAX=0x%x ECX=0x%x\n", func, index);
    
    // Do CPUID.
    X86ISA::ISA *isa = dynamic_cast<X86ISA::ISA *>(tc->getIsaPtr());
    X86ISA::CpuidResult result;
    isa->cpuid->doCpuid(tc, func, index, result);

    // Set RAX, RBX, RCX, RDX.
    tc->setReg(X86ISA::int_reg::Rax, result.rax);
    tc->setReg(X86ISA::int_reg::Rbx, result.rbx);
    tc->setReg(X86ISA::int_reg::Rdx, result.rdx);
    tc->setReg(X86ISA::int_reg::Rcx, result.rcx);
}

void
CPU::serializeThread(CheckpointOut &cp, ThreadID tid) const
{
    assert(tid == 0);
    thread->serialize(cp);
}

std::string
CPU::executePinCommand(const std::string &command)
{
    fatal_if(!isPinRunning(), "PinCPU has not been started up yet!\n");
    Message msg;
    msg.type = Message::ExecCommand;
    fatal_if(command.size() >= sizeof msg.command, "Command too long!\n");
    std::strcpy(msg.command, command.c_str());
    msg.send(reqFd);
    msg.recv(respFd);
    panic_if(msg.type != Message::CommandResult, "Received message other than CommandResult!\n");
    
    size_t rem = msg.command_result_size;
    std::string s;
    while (rem > 0) {
        char buf[1024];
        const ssize_t bytes = read(respFd, buf, std::min(rem, sizeof buf));
        if (bytes <= 0)
            panic("read failed: rem=%u\n", rem);
        s.insert(s.end(), &buf[0], &buf[bytes]);
        rem -= bytes;
    }
    assert(s.size() == msg.command_result_size);
    return s;
}

bool
CPU::isPinRunning() const
{
    if (pinPid < 0)
        return false;
    assert(reqFd >= 0);
    assert(respFd >= 0);
    return true;
}

}
}
