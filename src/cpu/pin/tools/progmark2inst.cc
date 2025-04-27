#include "progmark2inst.hh"

#include <iostream>
#include <fstream>
#include <unordered_set>
#include <vector>
#include <pin.H>

#include "client.hh"

using Count = long;

static KNOB<std::string> OutputFile(KNOB_MODE_WRITEONCE, "pintool", "progmark2inst", "", "Output path");
static KNOB<std::string> CountsFile(KNOB_MODE_WRITEONCE, "pintool", "progmark2inst-counts", "", "Input path");
static KNOB<std::string> MarkerFile(KNOB_MODE_WRITEONCE, "pintool", "progmark2inst-markers", "",
                                    "Path to progress marker file (instruction address list)");

static std::ofstream out;
static std::unordered_set<ADDRINT> progmarks;
static std::vector<Count> progmarker_counts;
static std::vector<Count> inst_counts;
static Count cur_progmarker_count = 0;
static Count target_progmarker_count;
static Count cur_inst_count = 0;

static void
UpdateInstCount(uint64_t num_insts)
{
    cur_inst_count += num_insts;
}

static void
UpdateProgmarkCount(uint64_t num_progmarks)
{
    cur_progmarker_count += num_progmarks;
    if (cur_progmarker_count >= target_progmarker_count && target_progmarker_count > 0) {
        inst_counts.push_back(cur_inst_count);
        assert(inst_counts.size() <= progmarker_counts.size());
        if (inst_counts.size() < progmarker_counts.size()) {
            target_progmarker_count = progmarker_counts[inst_counts.size()];
        } else {
            target_progmarker_count = 0;
        }
    }
}

static void
InstrumentBBL(BBL bbl)
{
    Count num_progmarks = 0;
    for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        if (progmarks.count(INS_Address(ins)))
            ++num_progmarks;
    BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR) UpdateInstCount,
                   IARG_UINT64, (uint64_t) BBL_NumIns(bbl),
                   IARG_END);
    if (num_progmarks > 0) {
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR) UpdateProgmarkCount,
                       IARG_UINT64, (uint64_t) num_progmarks,
                       IARG_END);
    }
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
    assert(inst_counts.size() <= progmarker_counts.size());
    if (inst_counts.size() < progmarker_counts.size()) {
        std::cerr << "progmark2inst: fatal error: missing inst counts on exit\n";
        return;
    }

    for (size_t i = 0; i < inst_counts.size(); ++i) {
        out << progmarker_counts[i] << " " << inst_counts[i] << "\n";
    }

    // Print out totals at end.
    out << cur_progmarker_count << " " << cur_inst_count << "\n";

    out.close();
}

// TODO: Rename to waypoint2inst.
bool progmark2inst_register()
{
    const bool out_empty = OutputFile.Value().empty();
    const bool counts_empty = CountsFile.Value().empty();
    const bool markers_empty = MarkerFile.Value().empty();
    if (out_empty && counts_empty && markers_empty)
        return true;

    if (out_empty || counts_empty || markers_empty) {
        std::cerr << "progmark2inst: input or output path not provided\n";
        return false;
    }

    out.open(OutputFile.Value());
    if (!out) {
        std::cerr << "progmark2inst: failed to open output file\n";
        return false;
    }

    std::ifstream counts_file(CountsFile.Value());
    if (!counts_file) {
        std::cerr << "progmark2inst: failed to open input file\n";
        return false;
    }
    Count count;
    while (counts_file >> count)
        progmarker_counts.push_back(count);
    counts_file.close();
    std::sort(progmarker_counts.begin(), progmarker_counts.end());
    if (progmarker_counts.size() < 1) {
        std::cerr << "progmark2inst: empty input!\n";
        return false;
    }
    std::cerr << "progmark2inst: parsed " << progmarker_counts.size() << " waypoint counts\n";
    std::cerr.flush();
    
    target_progmarker_count = progmarker_counts.front();

    std::ifstream marker_file(MarkerFile.Value());
    if (!marker_file) {
        std::cerr << "progmark2inst: failed to open marker file\n";
        return false;
    }
    ADDRINT inst;
    marker_file >> std::hex;
    while (marker_file >> inst)
        progmarks.insert(inst);
    marker_file.close();
    if (progmarks.empty()) {
        std::cerr << "progmark2inst: empty input!\n";
        return false;
    }

    TRACE_AddInstrumentFunction(InstrumentTRACE, nullptr);
    PIN_AddFiniFunction(Finish, nullptr);

    return true;
}
