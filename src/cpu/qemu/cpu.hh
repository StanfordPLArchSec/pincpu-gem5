#pragma once

#include <string>
#include <memory>

#include "cpu/base.hh"

namespace gem5
{

class BaseQemuCPUParams;
class SimpleThread;
class System;

namespace qemu
{

class CPU final : public BaseCPU
{
  public:
    CPU(const BaseQemuCPUParams &params);

    void init() override;
    void startup() override;
    void serializeThread(CheckpointOut &cp, ThreadID tid) const override;
    void activateContext(ThreadID tid = 0) override;

    class QemuRequestPort final : public RequestPort
    {
      public:
        QemuRequestPort(const std::string &name, CPU *cpu)
            : RequestPort(name), cpu(cpu)
        {
        }

      private:
        CPU *cpu;

        bool recvTimingResp(PacketPtr pkt) override;
        void recvReqRetry() override;
    };

    Port &getDataPort() override;
    Port &getInstPort() override;
    void wakeup(ThreadID tid) override;
    Counter totalInsts() const override { fatal("unimplemented\n"); }
    Counter totalOps() const override { fatal("unimplemented\n"); }

    void tick();

    enum Status
    {
        Idle,
        Running,
    };

  private:
    std::unique_ptr<SimpleThread> thread;
    ThreadContext *tc;
    EventFunctionWrapper tickEvent;
    Status _status;
    QemuRequestPort dataPort;
    QemuRequestPort instPort;

    // QEMU paths.
    std::string qemuExe;

    pid_t qemuPid = -1;
    int reqFd = -1;
    int respFd = -1;
    System *system;
};

}

}
