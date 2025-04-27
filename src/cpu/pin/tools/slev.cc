// TODO: Don't dump the last interval, or at least extend it with some special kind
// of marker to indicate it's the last.

#include "slev.hh"

#include <fstream>
#include <iostream>
#include <list>
#include <cassert>
#include <unordered_set>
#include <pin.H>

#include "client.hh"

static KNOB<std::string> OutputFile(KNOB_MODE_WRITEONCE, "pintool", "slev", "", "Collect source location edge vectors");
static KNOB<unsigned long> IntervalSize(KNOB_MODE_WRITEONCE, "pintool", "slev-interval", "0", "SLEV interval size");
static KNOB<std::string> ProgressMarkerFile(KNOB_MODE_WRITEONCE, "pintool", "slev-progmark", "",
                                            "Path to progress marker file (instruction address list)");

using Count = long;

static std::ofstream out;
static std::unordered_set<ADDRINT> progmarks; // TODO: Rename to waypoints.
static long interval_size = 0;

static long prev_progmarks = 0; // TODO: Rename to waypoints.
static long total_progmarks = 0; // TODO: Rename to waypoints.
static long cur_insts = 0;
static long prev_insts = 0;
static long total_insts = 0;
static int num_intervals = 0;

struct Block {
    long id;
    uint64_t hits;

    Block(long id)
        : id(id), hits(0)
    {
    }

    void
    reset()
    {
        hits = 0;
    }
};

static std::list<Block> blocks;

static void
DumpInterval()
{
    out << "T";
    for (auto &block : blocks) {
        if (block.hits) {
            out << " :" << block.id << ":" << block.hits;
            block.reset();
        }
    }
    out << "\n# interval=" << num_intervals << " insts=" << prev_insts << "," << total_insts << " progmarks=" << prev_progmarks << "," << total_progmarks << "\n";

    // Update global counters.
    prev_progmarks = total_progmarks;
    prev_insts = total_insts;
    total_insts += cur_insts;
    cur_insts -= interval_size;
    cur_insts = 0; // TODO: Maybe just subtract interval?
    ++num_intervals;
}

static void
UpdateProgmarkCount(uint64_t num_progmarks)
{
    total_progmarks += num_progmarks;
    if (cur_insts >= interval_size)
        DumpInterval();
}

static void
UpdateInstCount(uint64_t *hits, uint64_t num_insts)
{
    *hits += num_insts;
    cur_insts += num_insts;
}

static void
InstrumentBBL(BBL bbl)
{
    // TODO: Merge with code in progmark2inst.cc
    long num_progmarks = 0;
    for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        if (progmarks.count(INS_Address(ins)))
            ++num_progmarks;
    blocks.emplace_back(blocks.size() + 1);
    Block *block = &blocks.back();
    BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR) UpdateInstCount,
                   IARG_PTR, &block->hits,
                   IARG_UINT64, (uint64_t) BBL_NumIns(bbl),
                   IARG_END);
    BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR) UpdateProgmarkCount,
                   IARG_UINT64, (uint64_t) num_progmarks,
                   IARG_END);
}

static void
InstrumentTRACE(TRACE trace, void *)
{
    if (IsKernelCode(trace))
        return;
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
        InstrumentBBL(bbl);
}

static void
Finish(int32_t code, void *)
{
    DumpInterval();
    out.close();
}

bool
slev_register()
{
    if (OutputFile.Value().empty())
        return true;

    out.open(OutputFile.Value());
    if (!out) {
        std::cerr << "slev: failed to open output file\n";
        return false;
    }

    if (IntervalSize.Value() == 0) {
        std::cerr << "slev: -slev-interval: required\n";
        return false;
    }

    if (ProgressMarkerFile.Value().empty()) {
        std::cerr << "slev: -slev-progmark: required\n";
        return false;
    }

    // Parse progress markers.
    std::ifstream progmark_f(ProgressMarkerFile.Value());
    if (!progmark_f) {
        std::cerr << "slev: failed to open progress marker file\n";
        return false;
    }
    progmark_f >> std::hex;
    ADDRINT inst;
    while (progmark_f >> inst)
        progmarks.insert(inst);

    // Set interval size.
    interval_size = IntervalSize.Value();

    // Register callbacks.
    TRACE_AddInstrumentFunction(InstrumentTRACE, nullptr);
    PIN_AddFiniFunction(Finish, nullptr);

    return true;
}
