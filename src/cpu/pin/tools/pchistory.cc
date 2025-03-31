#include <vector>
#include <iostream>
#include <pin.H>

#include "plugin.hh"
#include "ringbuf.hh"
#include "client.hh"

namespace {

KNOB<bool> enable(KNOB_MODE_WRITEONCE, "pintool", "pchist", "0", "Enable PC recent history collection");

RingBuffer<ADDRINT, 16> pc_hist(0);

void
Analyze(ADDRINT pc)
{
    pc_hist.push(pc);
}

void
Instrument(TRACE trace, void *)
{
    if (IsKernelCode(trace))
        return;
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR) Analyze,
                       IARG_INST_PTR,
                       IARG_END);
}

void
DumpHistory()
{
    std::vector<ADDRINT> hist;
    pc_hist.get(std::back_inserter(hist));
    std::cerr << "history:";
    for (ADDRINT pc : hist)
        std::cerr << " 0x" << std::hex << pc;
    std::cerr << "\n";
}

void
Finish(int32_t code, void *)
{
    if (!code)
        return;
    DumpHistory();
}

struct PCHistoryPlugin final : Plugin
{
    bool
    enabled() const override
    {
        return enable.Value();
    }

    bool
    reg() override
    {
        TRACE_AddInstrumentFunction(Instrument, nullptr);
        PIN_AddFiniFunction(Finish, nullptr);
        return true;
    }
};

}
