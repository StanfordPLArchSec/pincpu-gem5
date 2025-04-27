from m5.defines import buildEnv
from m5.objects.BaseCPU import BaseCPU
from m5.params import *
from m5.SimObject import *
import os

class BasePinCPU(BaseCPU):
    type = "BasePinCPU"
    cxx_header = "cpu/pin/cpu.hh"
    cxx_class = "gem5::pin::CPU"

    @classmethod
    def memory_mode(cls):
        return "atomic"

    @classmethod
    def support_take_over(cls):
        return False

    @cxxMethod
    def executePinCommand(command):
        pass

    pinExe = Param.String(os.path.join(buildEnv["PIN_DIR"], 'pin'), "Path to Intel Pin executable")
    pinKernel = Param.String(buildEnv["PIN_KERNEL"], "Path to guest Pin kernel")
    pinTool = Param.String(buildEnv["PIN_CLIENT"], "Path to host PinTool")
    pinToolArgs = Param.String("", "Arguments to pass to PinTool")
    pinArgs = Param.String("", "Arguments to pass to Pin")

    # FIXME: Remove.
    countInsts = Param.Bool(False, "Enable instruction counting (moderate performance penalty)")
    traceInsts = Param.Bool(False, "Enable instruction tracing (huge performance penalty)")
    # FIXME: Remove
    enableBBV = Param.Bool(False, "Enable basic block profiling (e.g., for SimPoints)")
    # FIXME: Remove.
    interval = Param.Unsigned(50000000, "Basic block profiling interval (default: 50M instructions)")

    # FIXME: Remove.
    def addSimPointProbe(self, interval: int):
        self.enableBBV = True
        self.interval = interval
