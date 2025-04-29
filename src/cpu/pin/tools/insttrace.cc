#include <pin.H>
#include <iostream>

#include "plugin.hh"
#include "client.hh"

namespace {

KNOB<bool> enable(KNOB_MODE_WRITEONCE, "pintool", "insttrace", "0", "Enable instruction tracing");

void
Analyze(ADDRINT pc)
{
    std::cerr << "[insttrace] " << std::hex << pc << std::endl;
}

void
Instrument(INS ins, void *)
{
    if (IsKernelCode(ins))
        return;
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) Analyze,
                   IARG_INST_PTR,
                   IARG_END);
}

struct InstTracePlugin final : Plugin
{
    const char *name() const override { return "insttrace"; }

    bool
    enabled() const override
    {
        return enable.Value();
    }

    bool
    reg() override
    {
        INS_AddInstrumentFunction(Instrument, nullptr);
        return true;
    }
} plugin;

}
