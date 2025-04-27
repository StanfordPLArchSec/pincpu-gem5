#include "instcount.hh"

#include <pin.H>
#include <string>
#include <iostream>
#include "client.hh"
#include "plugin.hh"
#include "breakpoint.hh"

static KNOB<bool> enable(KNOB_MODE_WRITEONCE, "pintool", "instcount", "0", "Enable instruction counting");

// TODO: Make this static.
ADDRINT instcount;

static void
Analyze(ADDRINT n)
{
    instcount += n;
}

static void
Instrument(TRACE trace, void *)
{
    if (IsKernelCode(trace))
        return;
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR) Analyze,
                       IARG_ADDRINT, BBL_NumIns(bbl),
                       IARG_END);
    }
}

static void
Finish(int32_t code, void *)
{
    std::cerr << "instcount=" << std::dec << instcount << std::endl;
}

namespace {
struct InstCountPlugin final : Plugin
{
    const char *name() const override { return "instcount"; }

    int priority() const override { return 1; }

    bool
    enabled() const override
    {
        return enable.Value();
    }
    
    bool
    reg() override
    {
        assert(enabled());
        TRACE_AddInstrumentFunction(Instrument, nullptr);
        RegisterCounter("inst", &instcount);
        PIN_AddFiniFunction(Finish, nullptr);
        return true;
    }

    bool
    command(const std::string &cmd, const std::vector<std::string> &args, std::string &result) override
    {
        if (cmd == "instcount") {
            result = std::to_string(instcount);
            return true;
        }

        return false;
    }
} plugin;
}
