#include "util.hh"

#include <cstdio>
#include <string>

std::string
getBlockName(BBL bbl)
{
    std::string name;
    bool first = true;
    for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {
        char buf[256];
        std::sprintf(buf, "%s%lx", first ? "" : ",", INS_Address(ins));
        name += buf;
        first = false;
    }
    return name;
}
