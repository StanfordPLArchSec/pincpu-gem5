from m5.objects import (
    TAGE_SC_L_64KB,
    X86O3CPU,
)


class GoldenCove(X86O3CPU):
    def __init__(self, *args, **kwargs):
        X86O3CPU.__init__(self, *args, **kwargs)
        self.numROBEntries = 512
        self.fetchWidth = 6
        self.decodeWidth = 6
        self.renameWidth = 6
        self.LQEntries = 192
        self.SQEntries = 114
        self.numPhysIntRegs = 280
        self.numPhysFloatRegs = (
            332 * 2
        )  # Because gem5 float regs are 64-bit but Golden Cove's floats are likely at least 128 bit?
        self.numIQEntries = 97 + 70 + 38
        self.numPhysCCRegs = self.numPhysIntRegs * 5
        self.branchPred = TAGE_SC_L_64KB()

        self.numPhysMatRegs = 256
        self.numPhysVecRegs = 256

    def is_pcore(self) -> bool:
        return True

    def is_ecore(self) -> bool:
        return False


class Gracemont(X86O3CPU):
    def __init__(self, *args, **kwargs):
        X86O3CPU.__init__(self, *args, **kwargs)
        self.numROBEntries = 256
        self.fetchWidth = 6
        self.decodeWidth = 6
        self.renameWidth = 5
        self.LQEntries = 80
        self.SQEntries = 50
        self.numPhysIntRegs = 214
        self.numPhysFloatRegs = (
            207 * 2
        )  # Because gem5 float regs are 64-bit, and Golden Cove's floats are 128 bit
        self.numIQEntries = 15 + 16 + 16 + 15 + 42 + 22 + 18 + 35
        self.numPhysCCRegs = self.numPhysIntRegs * 5
        self.branchPred = TAGE_SC_L_64KB()

        self.numPhysMatRegs = 256
        self.numPhysVecRegs = 256

    def is_pcore(self) -> bool:
        return False

    def is_ecore(self) -> bool:
        return True
