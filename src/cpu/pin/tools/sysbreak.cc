#include <set>
#include <iostream>

#include "plugin.hh"
#include "client.hh"

static std::set<ADDRINT> sysbreaks;

static void
Analyze(ADDRINT sysno, CONTEXT *ctx)
{
    if (!sysbreaks.erase(sysno))
        return;
    // TODO: Factor this out into shared code for breaking.
    RunResult result;
    result.result = result.RUNRESULT_BREAK;
    std::cerr << "instbreak: switching to kernel\n";
    ContextSwitchToKernel(ctx, result);
    PIN_ExecuteAt(ctx);
}

static void
Instrument(INS ins, void *)
{
    if (IsKernelCode(ins) || !INS_IsSyscall(ins))
        return;

    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) Analyze,
                   IARG_SYSCALL_NUMBER,
                   IARG_CONTEXT,
                   IARG_END);
}

namespace {
struct SyscallBreakpointPlugin final : Plugin
{
    const char *name() const override { return "sysbreak"; }

    int priority() const override { return -1; }

    bool enabled() const override { return true; }

    bool
    reg() override
    {
        INS_AddInstrumentFunction(Instrument, nullptr);
        return true;
    }

    bool
    command(const std::string &cmd, const std::vector<std::string> &args, std::string &result) override
    {
        if (cmd != "sysbreak")
            return false;

        if (args.size() != 1) {
            std::cerr << "sysbreak: error: bad usage\n";
            std::cerr << "usage: sysbreak <sysno>\n";
            std::abort(); // TODO: Add return value for bad usage.
        }

        const ADDRINT sysno = std::stoull(args.at(0));
        sysbreaks.insert(sysno);
        return true;
    }
} plugin;
}
