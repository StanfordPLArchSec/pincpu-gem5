#pragma once

#include <pin.H>
#include <fstream>
#include <string>

#include "ops.hh"

bool IsKernelCode(ADDRINT addr);
bool IsKernelCode(INS ins);
bool IsKernelCode(TRACE trace);
std::ostream &log();

const std::string *GetSymbol(ADDRINT addr);
void ContextSwitchToKernel(CONTEXT *ctx, RunResult result);
