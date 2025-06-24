from m5.defines import buildEnv
from m5.objects.BaseCPU import BaseCPU
from m5.params import *
from m5.SimObject import *

class BaseQemuCPU(BaseCPU):
    type = "BaseQemuCPU"
    cxx_header = "cpu/qemu/cpu.hh"
    cxx_class = "gem5::qemu::CPU"

    @classmethod
    def memory_mode(cls):
        return "atomic"

    @classmethod
    def support_take_over(cls):
        return False

    qemuExe = Param.String(None, "Path to QEMU binary")

