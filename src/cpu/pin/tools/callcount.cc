#include "plugin.hh"
#include "client.hh"

#include <iostream>

namespace
{

KNOB<bool> enable(KNOB_MODE_WRITEONCE, "pintool", "callcount", "0", "Enable call counting");

uint64_t callcount = 0;

void
inc()
{
    callcount += 1;
}

void
instrument(INS ins, void *)
{
    if (IsKernelCode(ins))
        return;
    if (!INS_IsCall(ins))
        return;
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) inc, IARG_END);
}

void
finish(int32_t, void *)
{
    std::cerr << "callcount=" << std::dec << callcount << std::endl;
}

struct CallCountPlugin final : Plugin
{
    const char *name() const override { return "callcount"; }
    int priority() const override { return 1; }
    bool enabled() const override { return enable.Value(); }

    bool
    reg() override
    {
        INS_AddInstrumentFunction(instrument, nullptr);
        PIN_AddFiniFunction(finish, nullptr);
        return true;
    }
} plugin;

}

