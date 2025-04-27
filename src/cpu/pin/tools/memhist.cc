#include <pin.H>
#include <array>
#include <iostream>
#include <algorithm>
#include <limits>

#include "plugin.hh"

namespace {

KNOB<bool> enable(KNOB_MODE_WRITEONCE, "pintool", "memhist", "0", "Enable load/store data histogram collection");

using hash_t = uint16_t;

std::array<ADDRINT, std::numeric_limits<hash_t>::max() + 1> hist;

hash_t
hash(ADDRINT value)
{
#if 0
    return static_cast<uint8_t>(value * 0x9e3779b9);
#elif 0
    hash_t hash = ((value >> 0) & 0xFFFF) ^ ((value >> 16) & 0xFFFF) ^ ((value >> 32) & 0xFFFF) ^
        ((value >> 48) & 0xFFFF);
    return hash;
#else
    return static_cast<hash_t>(value * 0x9e3779b9);
#endif
}

void
RecordLoadValue(ADDRINT value)
{
    ++hist[hash(value)];
}

void
InstrumentINS(INS ins, void *)
{
    if (INS_IsMemoryRead(ins) && INS_IsValidForIpointAfter(ins)) {
        const REG dst = INS_RegW(ins, 0);
        if (REG_valid(dst) && REG_valid_for_iarg_reg_value(dst)) {
            INS_InsertPredicatedCall(ins, IPOINT_AFTER,
                                     (AFUNPTR) RecordLoadValue,
                                     IARG_REG_VALUE, dst,
                                     IARG_END);
        }
    }
}

void
Dump(std::string &result)
{
    for (std::size_t i = 0; i < hist.size(); ++i)
        if (const auto count = hist[i])
            result += std::to_string(count) + ' ' + std::to_string(i) + '\n';
}

void
Reset()
{
    std::fill(hist.begin(), hist.end(), 0);
}

struct MemoryHistogramPlugin final : Plugin
{
    const char *name() const override { return "memhist"; }

    int priority() const override { return 1; }

    bool
    enabled() const override
    {
        return enable.Value();
    }

    bool
    reg() override
    {
        INS_AddInstrumentFunction(InstrumentINS, nullptr);
        return true; 
    }

    void
    printUsage() const
    {
        std::cerr << "usage: memhist (dump|reset)\n";
    }

    bool
    command(const std::string &cmd, const std::vector<std::string> &args, std::string &result) override
    {
        if (cmd != "memhist")
            return false;

        if (args.empty()) {
            printUsage();
            std::abort();
        }

        const std::string &subcmd = args.at(0);
        if (subcmd == "dump") {
            Dump(result);
            return true;
        } else if (subcmd == "reset") {
            Reset();
            return true;
        }

        printUsage();
        std::abort();
    }
} plugin;

}
