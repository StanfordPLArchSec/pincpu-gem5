#include <pin.H>
#include <vector>
#include <iostream>
#include <map>

#include "client.hh"
#include "plugin.hh"

namespace
{

KNOB<bool> enable(KNOB_MODE_WRITEONCE, "pintool", "bbehist", "0", "Enable basic block edge histogram collection");
REG scratch_reg;

#if 0
std::vector<ADDRINT>
getInstVec(BBL bbl)
{
    std::vector<ADDRINT> insts;
    for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        insts.push_back(INS_Address(ins));
    return insts;
}
#endif

std::map<ADDRINT, std::map<ADDRINT, uint64_t>> count;

void
increment(uint64_t *counter)
{
    *counter += 1;
}

void
InstrumentTRACE(TRACE trace, void *)
{
    if (IsKernelCode(trace))
        return;
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
        INS tail = BBL_InsTail(bbl);
        if (INS_IsDirectControlFlow(tail)) {
            const ADDRINT src = INS_Address(tail);
            const ADDRINT taken_target = INS_DirectControlFlowTargetAddress(tail);
            uint64_t *taken_count = &count[src][taken_target];
            // Taken path.
            INS_InsertCall(tail, IPOINT_TAKEN_BRANCH, (AFUNPTR) increment,
                           IARG_PTR, taken_count,
                           IARG_END);
            // Not-taken path.
            if (INS_HasFallThrough(tail)) {
                const ADDRINT nottaken_target = INS_NextAddress(tail);
                uint64_t *nottaken_count = &count[src][nottaken_target];
                INS_InsertCall(tail, IPOINT_AFTER, (AFUNPTR) increment,
                               IARG_PTR, nottaken_count,
                               IARG_END);
            }
        } else if (INS_IsIndirectControlFlow(tail)) {
            // TODO
        }
    }
}

    

struct BasicBlockEdgeHistogramPlugin final : Plugin
{
    const char *name() const override { return "bbehist"; }
    int priority() const override { return 1; }

    bool
    enabled() const override
    {
        return enable.Value();
    }

    bool
    reg() override
    {
        scratch_reg = PIN_ClaimToolRegister();
        TRACE_AddInstrumentFunction(InstrumentTRACE, nullptr);
        return true;
    }

    bool
    command(const std::string &cmd, const std::vector<std::string> &args, std::string &result) override
    {
        if (cmd != name())
            return false;
        if (args.at(0) == "dump") {
            // Dump(result);
            std::cerr << "TODO\n";
            std::abort();
            return true;
        }

        std::cerr << name() << ": error: bad usage\n";
        std::cerr << "usage: " << name() << " dump\n";
        std::abort();
    }
} plugin;

}
