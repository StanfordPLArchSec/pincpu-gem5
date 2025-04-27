#include "waypointcount.hh"

#include <string>
#include <unordered_set>
#include <pin.H>
#include <iostream>

#include "plugin.hh"
#include "client.hh"
#include "waypoints.hh"
#include "breakpoint.hh"

// TODO: This should only check for basic blocks that have a non-zero waypoint count.

static KNOB<bool> CountWaypoints(KNOB_MODE_WRITEONCE, "pintool", "waypointcount", "0", "Enable waypointcount plugin");

// TODO: Make this static.
ADDRINT waypointcount;

static void
Analyze(ADDRINT n)
{
    waypointcount += n;
}

static void
InstrumentBBL(BBL bbl)
{
    // Count number of static waypoints in the basic block.
    const ADDRINT num_waypoints = countStaticWaypoints(bbl);
    if (num_waypoints == 0)
        return;
    BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR) Analyze,
                   IARG_ADDRINT, num_waypoints,
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
    std::cerr << "waypointcount=" << std::dec << waypointcount << std::endl;
}

namespace {
struct WaypointCountPlugin final : Plugin
{
    const char *name() const override { return "waypointcount"; }

    int priority() const override { return 1; }

    bool
    enabled() const override
    {
        return CountWaypoints.Value();
    }

    bool
    reg() override
    {
        TRACE_AddInstrumentFunction(InstrumentTRACE, nullptr);
        RegisterCounter("waypoint", &waypointcount);
        PIN_AddFiniFunction(Finish, nullptr);
        return true;
    }

    bool
    command(const std::string &cmd, const std::vector<std::string> &args, std::string &result) override
    {
        if (cmd != "waypointcount")
            return false;
        result = std::to_string(waypointcount);
        return true;
    }
} plugin;
}
