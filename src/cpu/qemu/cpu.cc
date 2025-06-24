#include "cpu/qemu/cpu.hh"

#include <cstdlib>

#include "cpu/simple_thread.hh"
#include "params/BaseQemuCPU.hh"
#include "base/output.hh"

namespace gem5
{

namespace qemu
{

CPU::CPU(const BaseQemuCPUParams &params)
    : BaseCPU(params),
      tickEvent([this] { tick(); }, "BaseQemuCPU tick", false, Event::CPU_Tick_Pri),
      _status(Idle),
      dataPort(name() + ".dcache_port", this),
      instPort(name() + ".icache_port", this),
      qemuExe(params.qemuExe),
      qemuPid(-1),
      system(params.system)
{
    thread = std::make_unique<SimpleThread>(
        this, /*thread_num*/0, params.system,
        params.workload[0], params.mmu,
        params.isa[0], params.decoder[0]);
    thread->setStatus(ThreadContext::Halted);
    tc = thread->getTC();
    threadContexts.push_back(tc);
}

bool
CPU::QemuRequestPort::recvTimingResp(PacketPtr pkt)
{
    fatal("Unsupported: %s\n", __func__);
}

void
CPU::QemuRequestPort::recvReqRetry()
{
    fatal("Unsupported: %s\n", __func__);
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
    fatal("Unsupported: %s\n", __func__);
}

void
CPU::init()
{
    BaseCPU::init();
    fatal_if(numThreads != 1, "QemuCPU: multithreading not supported\n");
}

void
CPU::startup()
{
    BaseCPU::startup();

    // Create pipes for bidirectional communication.
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

    std::stringstream shm_path_ss;
    const auto &backing_store = system->getPhysMem().getBackingStore();
    fatal_if(backing_store.size() != 1, "Pin CPU supports only one backing store entry");
    const int shm_fd = backing_store[0].shmFd;
    fatal_if(shm_fd < 0, "Pin CPU requires shared memory backing store");
    shm_path_ss << "/dev/fd/" << shm_fd;
    const std::string shm_path = shm_path_ss.str();
    int reg_fd;
    if ((reg_fd = memfd_create("qemucpu-regfile", 0)) < 0)
        err(EXIT_FAILURE, "memfd_create");
    const std::string reg_path = std::string("/dev/fd/") + std::to_string(reg_fd);

    int shm_fd_flags;
    if ((shm_fd_flags = fcntl(shm_fd, F_GETFD)) < 0)
        fatal("fcntl FD_GETFD failed");
    shm_fd_flags &= ~FD_CLOEXEC;
    if (fcntl(shm_fd, F_SETFD, shm_fd_flags) < 0)
        fatal("fcntl FD_SETFD failed");

    qemuPid = fork();
    if (qemuPid < 0) {
        fatal("fork: %s\n", std::strerror(errno));
    } else if (qemuPid == 0) {
        // Create log file for this fucking mess.
        // It will be for the kernel.
        const std::string kernout_path = simout.resolve("kernout.txt");
        const int kernout_fd = open(kernout_path.c_str(), O_WRONLY | O_APPEND | O_TRUNC | O_\
CREAT, 0664);
        if (kernout_fd < 0)
            panic("Failed to create kernel.log\n");
        if (dup2(kernout_fd, STDOUT_FILENO) < 0)
            panic("dup2 failed\n");

        const std::string kernerr_path = simout.resolve("kernerr.txt");
        const int kernerr_fd = open(kernerr_path.c_str(), O_WRONLY | O_APPEND | O_TRUNC | O_\
CREAT, 0664);
        if (kernerr_fd < 0)
            panic("Failed to create kernerr.txt");
        if (dup2(kernerr_fd, STDERR_FILENO) < 0)
            panic("dup2 failed\n");

        std::vector<std::string> qemu_subcommand;
        auto it = std::back_inserter(qemu_subcommand);

        // QEMU executable.
        *it++ = qemuExe;

        // QEMU args.
        setenv("QEMUCPU_IN", req_path, true);
        setenv("QEMUCPU_OUT", resp_path, true);
        setenv("QEMUCPU_MEM", shm_path.c_str(), true);
        setenv("QEMUCPU_REG", reg_path.c_str(), true);

        std::vector<char *> qemu_subcommand_c;
        for (const std::string &s : qemu_subcommand)
            qemu_subcommand_c.push_back(const_cast<char *>(s.c_str()));
        qemu_subcommand_c.push_back(nullptr);

        dprintf(kernout_fd, "Starting QEMU:");
        for (const std::string &s : qemu_subcommand)
            dprintf(kernout_fd, " %s", s.c_str());
        dprintf(kernout_fd, "\n");

        execvp(qemu_subcommand_c[0], qemu_subcommand_c.data());
        fatal("execvp failed: %s\n", qemu_subcommand[0]);
    }
    
    fatal("unimplemented\n");
}

void
CPU::tick()
{
    fatal("unimplemented\n");
}

void
CPU::activateContext(ThreadID tid)
{
    panic_if(tid != 0, "bad tid\n");
    assert(thread);

    schedule(tickEvent, clockEdge(Cycles(0)));
    _status = Running;
}

void
CPU::serializeThread(CheckpointOut &cp, ThreadID tid) const
{
    fatal("unimplemented\n");
}

}

}
