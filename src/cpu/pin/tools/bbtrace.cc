#include <pin.H>
#include <fstream>
#include <string>
#include <iostream>

#include "plugin.hh"
#include "client.hh"
#include "xxhash.hh"
#include "util.hh"

namespace {

// TODO: Should make these enabled by the gem5 configuration script.
KNOB<std::string> path(KNOB_MODE_WRITEONCE, "pintool", "bbtrace", "", "Write basic block trace to path");
std::ofstream os;
BUFFER_ID buffer;

uint64_t fill_counter = 0;
uint64_t flush_counter = 0;

static void
inc()
{
    fill_counter += 1;
}

static void
InstrumentBBL(BBL bbl)
{
    const std::string name = getBlockName(bbl);
    const uint32_t hash = XXHash32::hash(name.data(), name.size(), 0);
    INS_InsertFillBuffer(BBL_InsHead(bbl), IPOINT_BEFORE, buffer,
                         IARG_UINT32, hash, 0,
                         IARG_END);
    INS_InsertCall(BBL_InsHead(bbl), IPOINT_BEFORE, (AFUNPTR) inc,
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

static void *
trace_callback(BUFFER_ID, THREADID, const CONTEXT *, void *buf_, uint64_t nitems, void *)
{
    uint32_t *buf = (uint32_t *) buf_;
    if (nitems == 0) {
        for (; buf[nitems]; ++nitems)
            ;
        if (nitems)
            std::cerr << "[!] bbtrace: warning: detected falsely empty buffer, including elements past end\n";
    }
    os.write((const char *) buf, nitems * sizeof(uint32_t));
    std::fill_n(buf, nitems, 0);
    flush_counter += nitems;
    std::cerr << "[*] bbtrace: flushing buffer with " << std::dec << nitems << "\n";
    return buf;
}

static void
finish(int32_t code, void *)
{
    os.flush();
    os.close();
    std::cerr << "bbtrace-fills " << std::dec << fill_counter << "\n";
    std::cerr << "bbtrace-flushes " << std::dec << flush_counter << "\n";
}

struct BBTracePlugin final : Plugin
{
    const char *name() const override { return "bbtrace"; }

    int priority() const override { return 1; }

    bool
    enabled() const override
    {
        return !path.Value().empty();
    }

    bool
    reg() override
    {
        TRACE_AddInstrumentFunction(InstrumentTRACE, nullptr);
        buffer = PIN_DefineTraceBuffer(sizeof(UINT32), 32, trace_callback, nullptr);
        os.open(path.Value(), os.binary);
        if (!os) {
            std::cerr << "[!] error: bbtrace: failed to open file: " << path.Value() << "\n";
            return false;
        }
        PIN_AddFiniFunction(finish, nullptr);
        return static_cast<bool>(os);
    }
        
} plugin;

}
