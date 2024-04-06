from m5.objects import X86O3CPU, TAGE_SC_L_64KB

class AlderLake_GoldenCove(X86O3CPU):
    def __init__(self, *args, **kwargs):
        X86O3CPU.__init__(self, *args, **kwargs)
        self.numROBEntries = 512
        self.fetchWidth = 6
        self.decodeWidth = 6
        self.renameWidth = 6
        self.LQEntries = 192
        self.SQEntries = 114
        self.numPhysIntRegs = 280
        self.numPhysFloatRegs = 332
        self.numIQEntries = 2 * 72
        self.numPhysCCRegs = self.numPhysIntRegs * 5
        self.branchPred = TAGE_SC_L_64KB()
