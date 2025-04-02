#include <pin.H>
#include <iostream>
#include <fstream>

#include "plugin.hh"
#include "client.hh"
#include "breakpoint.hh"

namespace {

KNOB<std::string> CountAddrs(KNOB_MODE_WRITEONCE, "pintool", "addrcount", "", "Enable addrcount plugin (path)");

constexpr std::size_t addrsize = 1ULL << 32;
std::bitset<addrsize> addrmask;
ADDRINT addrcount;

void
Analyze(ADDRINT addr)
{
    addrcount += addrmask[static_cast<uint32_t>(addr)];
}

void
InstrumentINS(INS ins, void *)
{
    if (IsKernelCode(ins))
        return;
    for (uint32_t memop = 0; memop < INS_MemoryOperandCount(ins); ++memop) {
        if (INS_MemoryOperandIsRead(ins, memop)) {
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) Analyze,
                                     IARG_MEMORYOP_EA, memop,
                                     IARG_END);
        }
    }
}

void
Finish(int32_t code, void *)
{
    std::cerr << "addrcount=" << std::dec << addrcount << std::endl;
}

struct AddressCountPlugin final : Plugin
{
    const char *name() const override { return "addrcount"; }

    int priority() const override { return 1; }

    bool
    enabled() const override
    {
        return !CountAddrs.Value().empty();
    }

    bool
    reg() override
    {
        INS_AddInstrumentFunction(InstrumentINS, nullptr);

        RegisterCounter("addr", &addrcount);

        PIN_AddFiniFunction(Finish, nullptr);

        // Parse addresses into addrmask.
        std::ifstream is(CountAddrs.Value());
        uint32_t addr;
        is >> std::hex;
        while (is >> addr)
            addrmask[addr] = true;
        
        return true;
    }

    bool
    command(const std::string &cmd, const std::vector<std::string> &args, std::string &result) override
    {
        if (cmd != "addrcount")
            return false;
        result = std::to_string(addrcount);
        return true;
    }
} plugin;

}
