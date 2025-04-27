#include <pin.H>
#include <array>
#include <iostream>

#include "plugin.hh"
#include "client.hh"

namespace {

KNOB<bool> enable(KNOB_MODE_WRITEONCE, "pintool", "addrhist", "0", "Enable address histogram");

constexpr std::size_t max_addr = 1ULL << 32;
std::vector<uint32_t> hist;

uint32_t
hash(ADDRINT addr)
{
    return static_cast<uint32_t>(addr);
}

void
RecordLoadAddress(ADDRINT addr)
{
    ++hist[hash(addr)];
}

void
InstrumentINS(INS ins, void *)
{
    if (IsKernelCode(ins))
        return;
    for (uint32_t memop = 0; memop < INS_MemoryOperandCount(ins); ++memop) {
        if (INS_MemoryOperandIsRead(ins, memop)) {
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) RecordLoadAddress,
                                     IARG_MEMORYOP_EA, memop,
                                     IARG_END);
        }
    }
}

void
Dump(std::string &result)
{
    for (std::size_t addr = 0; addr < max_addr; ++addr) {
        if (const uint32_t count = hist[addr]) {
            char buf[32];
            sprintf(buf, "%#zx", addr);
            result += buf;
            result += ' ';
            result += std::to_string(count);
            result += '\n';
        }
    }
}

struct AddressHistogramPlugin final : Plugin
{
    const char *name() const override { return "addrhist"; }
    int priority() const override { return 1; }
    bool enabled() const override { return enable.Value(); }

    bool
    reg() override
    {
        INS_AddInstrumentFunction(InstrumentINS, nullptr);
        hist.resize(max_addr);
        return true;
    }

    void
    printUsage() const
    {
        std::cerr << "usage: addrhist dump\n";
    }

    bool
    command(const std::string &cmd, const std::vector<std::string> &args, std::string &result) override
    {
        if (cmd != "addrhist")
            return false;

        if (args.size() != 1 || args[0] != "dump") {
            printUsage();
            std::abort();
        }

        Dump(result);
        return true;
    }
    
} plugin;

}
