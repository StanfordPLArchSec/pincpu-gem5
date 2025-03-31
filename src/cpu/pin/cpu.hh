#pragma once

#include <memory>
#include <optional>
#include <set>

#include "cpu/base.hh"

namespace gem5
{

// Forward declarations.
class SimpleThread;
class BasePinCPUParams;
class System;

namespace pin
{

class CPU final : public BaseCPU
{
  public:
    CPU(const BasePinCPUParams &params);

    void init() override;
    void startup() override;

    void serializeThread(CheckpointOut &cp, ThreadID tid) const override;
    
    void activateContext(ThreadID tid = 0) override;

    class PinRequestPort final : public RequestPort
    {
      public:
        PinRequestPort(const std::string &name, CPU *cpu)
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
    Counter totalInsts() const override;
    Counter totalOps() const override;

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
    PinRequestPort dataPort;
    PinRequestPort instPort;

    // Pin paths.
    std::string pinExe;
    std::string pinKernel;
    std::string pinTool;
    std::vector<std::string> pinArgs;
    std::vector<std::string> pinToolArgs;
    
    // TODO: Consider abstracting the Pin process into its own class.
    pid_t pinPid = -1;
    int reqFd = -1;
    int respFd = -1;
    System *system;
    std::optional<Counter> ctrInsts;
    bool traceInsts;

    // Basic block profiling
    bool enableBBV;
    unsigned long interval;

    static const char *getPinRoot();
    const std::string& getPinTool() const;
    const std::string& getPinExe() const;
    const std::string& getDummyProg() const;
    static const char *getRequestPath();
    static const char *getResponsePath();

    void pinRun();

    void syncStateToPin(bool full);
    void syncStateFromPin(bool full);

    void syncSingleRegToPin(const char *name, const RegId &reg);
    void syncRegvalToPin(const char *name, const void *data, size_t size);

    template <typename T>
    void syncRegvalToPin(const char *name, T value);

    void syncRegvalFromPin(const char *name, void *data, size_t size);
    template <typename T>
    T syncRegvalFromPin(const char *name);
    void syncRegFromPin(const char *name, const RegId &reg);
    

    void handlePageFault(Addr vaddr);
    void handleSyscall();

    Tick doMMIOAccess(Addr paddr, void *data, int size, bool write);

    void handleCPUID();

    void haltContext();

    bool isPinRunning() const;

    void mapCode();

  public:
    std::string executePinCommand(const std::string &command);
};

}
}
