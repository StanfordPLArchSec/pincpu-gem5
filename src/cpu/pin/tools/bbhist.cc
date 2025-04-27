#include <vector>
#include <map>
#include <cassert>
#include <iostream>
#include <string>
#include <pin.H>

#include "plugin.hh"
#include "client.hh"

namespace {

KNOB<bool> enable(KNOB_MODE_WRITEONCE, "pintool", "bbhist", "0", "Enable basic block histogram collection");

std::vector<ADDRINT>
getInstVec(BBL bbl)
{
    std::vector<ADDRINT> insts;
    for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        insts.push_back(INS_Address(ins));
    return insts;
}

struct BlockData
{
    ADDRINT hits;
    std::string name;

    BlockData(BBL bbl)
        : hits(0)
    {
        bool first = true;
        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {
            char buf[256];
            std::sprintf(buf, "%s%lx", first ? "" : ",", INS_Address(ins));
            name += buf;
            first = false;
        }
    }
};

std::map<std::vector<ADDRINT>, BlockData> blocks;

BlockData &
getBlockData(BBL bbl)
{
    return blocks.emplace(getInstVec(bbl), bbl).first->second;
}

void
Analyze(ADDRINT *counter)
{
    ++*counter;
}

void
InstrumentBBL(BBL bbl)
{
    ADDRINT &counter = getBlockData(bbl).hits;
    BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR) Analyze,
                   IARG_PTR, &counter,
                   IARG_END);
}

void
InstrumentTRACE(TRACE trace, void *)
{
    if (IsKernelCode(trace))
        return;
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
        InstrumentBBL(bbl);
}

void
Dump(std::string &s)
{
    for (const auto &[insts, block] : blocks)
        if (block.hits)
            s += std::to_string(block.hits) + ' ' + block.name + '\n';
}

void
Reset()
{
    for (auto &[insts, block] : blocks)
        block.hits = 0;
}

struct BasicBlockHistogramPlugin final : Plugin
{
    const char *name() const override { return "bbhist"; }

    int priority() const override { return 1; }

    bool
    enabled() const override
    {
        return enable.Value();
    }

    bool
    reg() override
    {
        TRACE_AddInstrumentFunction(InstrumentTRACE, nullptr);
        return true;
    }

    bool
    command(const std::string &cmd, const std::vector<std::string> &args, std::string &result) override
    {
        if (cmd != "bbhist")
            return false;

        if (args.at(0) == "dump") {
            Dump(result);
            return true;
        } else if (args.at(0) == "reset") {
            Reset();
            return true;
        }

        std::cerr << "bbhist: error: bad usage\n";
        std::cerr << "usage: bbhist (dump|reset)\n";
        std::abort();
    }
} plugin;

}
