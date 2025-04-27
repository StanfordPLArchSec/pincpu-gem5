#pragma once

#include <unordered_set>
#include <pin.H>

extern std::unordered_set<ADDRINT> waypoints;

ADDRINT countStaticWaypoints(BBL bbl);
