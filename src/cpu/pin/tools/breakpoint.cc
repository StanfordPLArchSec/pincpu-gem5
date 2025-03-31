#include "breakpoint.hh"

#include <list>
#include <iostream>
#include <pin.H>

#include "ops.hh"
#include "client.hh"
#include "plugin.hh"

static std::map<std::string, const ADDRINT *> counters;
static std::map<const ADDRINT *, ADDRINT> breakpoints;

static void
SetBreakpoint(const ADDRINT *counter, ADDRINT target)
{
    // Remove instrumentation if we're adding a new type of breakpoint.
    if (!breakpoints.count(counter))
        PIN_RemoveInstrumentation();
    breakpoints[counter] = target;
}

static void
ClearBreakpoint(const ADDRINT *counter)
{
    breakpoints.at(counter) = std::numeric_limits<ADDRINT>::max();
}

static ADDRINT
AnalyzeIf(ADDRINT *target, const ADDRINT *counter)
{
    return *counter >= *target;
}

static void
AnalyzeThen(CONTEXT *ctx, const ADDRINT *counter)
{
    ClearBreakpoint(counter);
    RunResult result;
    result.result = result.RUNRESULT_BREAK;
    std::cerr << "instbreak: switching to kernel\n";
    ContextSwitchToKernel(ctx, result);
    PIN_ExecuteAt(ctx);
}

static void
Instrument(TRACE trace, void *)
{
    if (IsKernelCode(trace))
        return;
    for (const auto &[counter, target] : breakpoints) {
        TRACE_InsertIfCall(trace, IPOINT_BEFORE, (AFUNPTR) AnalyzeIf,
                           IARG_ADDRINT, &target,
                           IARG_PTR, counter,
                           IARG_END);
        TRACE_InsertThenCall(trace, IPOINT_BEFORE, (AFUNPTR) AnalyzeThen,
                             IARG_CONTEXT,
                             IARG_PTR, counter,
                             IARG_END);
    }
}

void
RegisterCounter(const std::string &name, const ADDRINT *counter)
{
    counters[name] = counter;
}

namespace {
struct BreakpointPlugin final : Plugin
{
    const char *name() const override { return "breakpoint"; }

    int priority() const override { return -1; }

    bool enabled() const override { return true; }

    bool
    reg() override
    {
        TRACE_AddInstrumentFunction(Instrument, nullptr);
        return true;
    }

    bool
    command(const std::string &cmd, const std::vector<std::string> &args, std::string &result) override
    {
        if (cmd != "breakpoint")
            return false;

        if (args.size() != 2) {
            std::cerr << "breakpoint: error: bad usage\n";
            std::cerr << "usage: breakpoint <counter> <target>\n";
            std::abort();
        }

        const ADDRINT target = std::stoull(args.at(1));
        const std::string &counter_name = args.at(0);
        const auto counter_it = counters.find(counter_name);
        if (counter_it == counters.end()) {
            std::cerr << "breakpoint: error: no such counter: " << counter_name << "\n";
            std::abort();
        }

        SetBreakpoint(counter_it->second, target);
        return true;
    }
} plugin;
}
