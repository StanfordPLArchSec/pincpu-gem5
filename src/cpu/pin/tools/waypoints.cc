#include "waypoints.hh"

#include <string>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <pin.H>

#include "plugin.hh"

static KNOB<std::string> WaypointsFile(KNOB_MODE_WRITEONCE, "pintool", "waypoints", "", "Path to waypoints (list of instruction addresses)");

std::unordered_set<ADDRINT> waypoints;

static bool
isWaypoint(INS ins)
{
    return waypoints.count(INS_Address(ins)) > 0;
}

ADDRINT
countStaticWaypoints(BBL bbl)
{
    ADDRINT n = 0;
    for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        if (isWaypoint(ins))
            ++n;
    return n;
}

namespace {
struct WaypointsPlugin final : Plugin
{
    const char *name() const override { return "waypoints"; }

    bool
    enabled() const override
    {
        return !WaypointsFile.Value().empty();
    }

    bool
    reg() override
    {
        // Parse the waypoints.
        std::ifstream f(WaypointsFile.Value());
        if (!f) {
            std::cerr << "waypoints: error: failed to open file: " << WaypointsFile.Value() << "\n";
            return false;
        }
        ADDRINT waypoint;
        while (f >> std::hex >> waypoint)
            waypoints.insert(waypoint);
        std::cerr << "waypoints: parsed " << waypoints.size() << " waypoints\n";

        return true; 
    }
} plugin;
}
