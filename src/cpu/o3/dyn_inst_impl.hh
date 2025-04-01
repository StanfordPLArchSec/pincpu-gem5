/*
 * Copyright (c) 2010-2011 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2004-2006 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CPU_O3_DYN_INST_IMPL_HH__
#define __CPU_O3_DYN_INST_IMPL_HH__

#include "arch/x86/faults.hh"
#include "cpu/o3/dyn_inst.hh"
#include "debug/O3PipeView.hh"
#include "debug/HFI.hh"

template <class Impl>
BaseO3DynInst<Impl>::BaseO3DynInst(const StaticInstPtr &staticInst,
                                   const StaticInstPtr &macroop,
                                   TheISA::PCState pc, TheISA::PCState predPC,
                                   InstSeqNum seq_num, O3CPU *cpu)
    : BaseDynInst<Impl>(staticInst, macroop, pc, predPC, seq_num, cpu)
{
    initVars();
}

template <class Impl>
BaseO3DynInst<Impl>::BaseO3DynInst(const StaticInstPtr &_staticInst,
                                   const StaticInstPtr &_macroop)
    : BaseDynInst<Impl>(_staticInst, _macroop)
{
    initVars();
}

template <class Impl>BaseO3DynInst<Impl>::~BaseO3DynInst()
{
#if TRACING_ON
    if (DTRACE(O3PipeView)) {
        Tick fetch = this->fetchTick;
        // fetchTick can be -1 if the instruction fetched outside the trace window.
        if (fetch != -1) {
            Tick val;
            // Print info needed by the pipeline activity viewer.
            DPRINTFR(O3PipeView, "O3PipeView:fetch:%llu:0x%08llx:%d:%llu:%s\n",
                     fetch,
                     this->instAddr(),
                     this->microPC(),
                     this->seqNum,
                     this->staticInst->disassemble(this->instAddr()));

            val = (this->decodeTick == -1) ? 0 : fetch + this->decodeTick;
            DPRINTFR(O3PipeView, "O3PipeView:decode:%llu\n", val);
            val = (this->renameTick == -1) ? 0 : fetch + this->renameTick;
            DPRINTFR(O3PipeView, "O3PipeView:rename:%llu\n", val);
            val = (this->dispatchTick == -1) ? 0 : fetch + this->dispatchTick;
            DPRINTFR(O3PipeView, "O3PipeView:dispatch:%llu\n", val);
            val = (this->issueTick == -1) ? 0 : fetch + this->issueTick;
            DPRINTFR(O3PipeView, "O3PipeView:issue:%llu\n", val);
            val = (this->completeTick == -1) ? 0 : fetch + this->completeTick;
            DPRINTFR(O3PipeView, "O3PipeView:complete:%llu\n", val);
            val = (this->commitTick == -1) ? 0 : fetch + this->commitTick;

            Tick valS = (this->storeTick == -1) ? 0 : fetch + this->storeTick;
            DPRINTFR(O3PipeView, "O3PipeView:retire:%llu:store:%llu\n", val, valS);
        }
    }
#endif
};


template <class Impl>
void
BaseO3DynInst<Impl>::initVars()
{
    this->_readySrcRegIdx.reset();

    _numDestMiscRegs = 0;

#if TRACING_ON
    // Value -1 indicates that particular phase
    // hasn't happened (yet).
    fetchTick = -1;
    decodeTick = -1;
    renameTick = -1;
    dispatchTick = -1;
    issueTick = -1;
    completeTick = -1;
    commitTick = -1;
    storeTick = -1;
#endif
}

template <class Impl>
Fault
BaseO3DynInst<Impl>::execute(){

    //HFI
    if (!checkHFICtrl(this->pc.instAddr())) {
        return std::make_shared<TheISA::HFIBoundsCheck>();
    }

    // @todo: Pretty convoluted way to avoid squashing from happening
    // when using the TC during an instruction's execution
    // (specifically for instructions that have side-effects that use
    // the TC).  Fix this.
    bool no_squash_from_TC = this->thread->noSquashFromTC;
    this->thread->noSquashFromTC = true;

    this->fault = this->staticInst->execute(this, this->traceData);

    this->thread->noSquashFromTC = no_squash_from_TC;

    return this->fault;
}

template <class Impl>
Fault
BaseO3DynInst<Impl>::initiateAcc()
{
    //HFI
    if (!checkHFICtrl(this->pc.instAddr()))
        return  std::make_shared<TheISA::HFIBoundsCheck>();

    // @todo: Pretty convoluted way to avoid squashing from happening
    // when using the TC during an instruction's execution
    // (specifically for instructions that have side-effects that use
    // the TC).  Fix this.
    bool no_squash_from_TC = this->thread->noSquashFromTC;
    this->thread->noSquashFromTC = true;

    this->fault = this->staticInst->initiateAcc(this, this->traceData);

    this->thread->noSquashFromTC = no_squash_from_TC;

    return this->fault;
}

template <class Impl>
Fault
BaseO3DynInst<Impl>::completeAcc(PacketPtr pkt)
{
    // @todo: Pretty convoluted way to avoid squashing from happening
    // when using the TC during an instruction's execution
    // (specifically for instructions that have side-effects that use
    // the TC).  Fix this.
    bool no_squash_from_TC = this->thread->noSquashFromTC;
    this->thread->noSquashFromTC = true;

    if (this->cpu->checker) {
        if (this->isStoreConditional()) {
            this->reqToVerify->setExtraData(pkt->req->getExtraData());
        }
    }

    this->fault = this->staticInst->completeAcc(pkt, this, this->traceData);

    this->thread->noSquashFromTC = no_squash_from_TC;

    return this->fault;
}


template <class Impl>
bool
BaseO3DynInst<Impl>::checkHFICtrl(Addr pc) {
    using namespace TheISA;

    bool is_inside_sandbox = this->readMiscReg(MISCREG_HFI_INSIDE_SANDBOX) != 0;

    if (!is_inside_sandbox) {
        return true;
    }

    MiscRegIndex hfi_regs_base[]          = { MISCREG_HFI_LINEAR_CODERANGE_1_BASE_MASK  , MISCREG_HFI_LINEAR_CODERANGE_2_BASE_MASK   };
    MiscRegIndex hfi_regs_offset_ignore[] = { MISCREG_HFI_LINEAR_CODERANGE_1_IGNORE_MASK, MISCREG_HFI_LINEAR_CODERANGE_2_IGNORE_MASK };
    MiscRegIndex hfi_regs_perm[]          = { MISCREG_HFI_LINEAR_CODERANGE_1_EXECUTABLE , MISCREG_HFI_LINEAR_CODERANGE_2_EXECUTABLE  };

    bool found = false;
    bool faulted = false;

    for (int i = 0; i < 2; i++) {
        doHFIMaskCheck(pc, hfi_regs_base[i], hfi_regs_offset_ignore[i], hfi_regs_perm[i], /* out */ found, /* out */ faulted);
        if (found) {
            break;
        }
    }

    // Addr rip = this->cpu->pcState(this->threadNumber).pc();
    // if (pc != rip) {
    //     printf("!!!!!Prefetch / Speculative Exec!!!!!!!\n");
    // } else {
    //     printf("!!!!!Sequential Exec!!!!!!!\n");
    // }

    if (faulted || !found) {
        // printf("HFI code mask oob\n");
        printHFIMetadata();
        return false;
    }

    return true;
}


template <class Impl>
void
BaseO3DynInst<Impl>::printHFIMetadata() {
    using namespace TheISA;

    DPRINTF(HFI, "HFI sandbox bounds metadata!\n");
    DPRINTF(HFI, "HFI_LINEAR_RANGE_1_READABLE: %" PRIu64 "\n",     (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_1_READABLE));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_1_WRITEABLE: %" PRIu64 "\n",    (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_1_WRITEABLE));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_1_RANGESIZETYPE: %" PRIu64 "\n",    (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_1_RANGESIZETYPE));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_1_BASE_ADDRESS_BASE_MASK: %" PRIu64 "\n", (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_1_BASE_ADDRESS_BASE_MASK));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_1_OFFSET_LIMIT_IGNORE_MASK: %" PRIu64 "\n",  (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_1_OFFSET_LIMIT_IGNORE_MASK));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_2_READABLE: %" PRIu64 "\n",     (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_2_READABLE));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_2_WRITEABLE: %" PRIu64 "\n",    (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_2_WRITEABLE));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_2_RANGESIZETYPE: %" PRIu64 "\n",    (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_2_RANGESIZETYPE));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_2_BASE_ADDRESS_BASE_MASK: %" PRIu64 "\n", (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_2_BASE_ADDRESS_BASE_MASK));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_2_OFFSET_LIMIT_IGNORE_MASK: %" PRIu64 "\n",  (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_2_OFFSET_LIMIT_IGNORE_MASK));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_3_READABLE: %" PRIu64 "\n",     (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_3_READABLE));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_3_WRITEABLE: %" PRIu64 "\n",    (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_3_WRITEABLE));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_3_RANGESIZETYPE: %" PRIu64 "\n",    (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_3_RANGESIZETYPE));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_3_BASE_ADDRESS_BASE_MASK: %" PRIu64 "\n", (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_3_BASE_ADDRESS_BASE_MASK));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_3_OFFSET_LIMIT_IGNORE_MASK: %" PRIu64 "\n",  (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_3_OFFSET_LIMIT_IGNORE_MASK));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_4_READABLE: %" PRIu64 "\n",     (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_4_READABLE));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_4_WRITEABLE: %" PRIu64 "\n",    (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_4_WRITEABLE));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_4_RANGESIZETYPE: %" PRIu64 "\n",    (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_4_RANGESIZETYPE));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_4_BASE_ADDRESS_BASE_MASK: %" PRIu64 "\n", (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_4_BASE_ADDRESS_BASE_MASK));
    DPRINTF(HFI, "HFI_LINEAR_RANGE_4_OFFSET_LIMIT_IGNORE_MASK: %" PRIu64 "\n",  (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_RANGE_4_OFFSET_LIMIT_IGNORE_MASK));
    DPRINTF(HFI, "HFI_LINEAR_CODERANGE_1_EXECUTABLE: %" PRIu64 "\n", (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_CODERANGE_1_EXECUTABLE));
    DPRINTF(HFI, "HFI_LINEAR_CODERANGE_1_BASE_MASK: %" PRIu64 "\n", (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_CODERANGE_1_BASE_MASK));
    DPRINTF(HFI, "HFI_LINEAR_CODERANGE_1_IGNORE_MASK: %" PRIu64 "\n", (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_CODERANGE_1_IGNORE_MASK));
    DPRINTF(HFI, "HFI_LINEAR_CODERANGE_2_EXECUTABLE: %" PRIu64 "\n", (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_CODERANGE_2_EXECUTABLE));
    DPRINTF(HFI, "HFI_LINEAR_CODERANGE_2_BASE_MASK: %" PRIu64 "\n", (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_CODERANGE_2_BASE_MASK));
    DPRINTF(HFI, "HFI_LINEAR_CODERANGE_2_IGNORE_MASK: %" PRIu64 "\n", (uint64_t) readMiscReg(MISCREG_HFI_LINEAR_CODERANGE_2_IGNORE_MASK));
    DPRINTF(HFI, "HFI_IS_TRUSTED_SANDBOX: %" PRIu64 "\n",          (uint64_t) readMiscReg(MISCREG_HFI_IS_TRUSTED_SANDBOX));
    DPRINTF(HFI, "HFI_EXIT_SANDBOX_HANDLER: %" PRIu64 "\n",        (uint64_t) readMiscReg(MISCREG_HFI_EXIT_SANDBOX_HANDLER));
    DPRINTF(HFI, "HFI_INSIDE_SANDBOX: %" PRIu64 "\n",              (uint64_t) readMiscReg(MISCREG_HFI_INSIDE_SANDBOX));
    DPRINTF(HFI, "HFI_EXIT_REASON: %" PRIu64 "\n",                 (uint64_t) readMiscReg(MISCREG_HFI_EXIT_REASON));
    DPRINTF(HFI, "HFI_EXIT_LOCATION: %" PRIu64 "\n",               (uint64_t) readMiscReg(MISCREG_HFI_EXIT_LOCATION));
}

template <class Impl>
Addr BaseO3DynInst<Impl>::doHFIStructuredMov(uint64_t segment_index,
    uint64_t scale,
    uint64_t index,
    uint64_t displacement,
    TheISA::MiscRegIndex reg_base_address,
    TheISA::MiscRegIndex reg_offset_limit,
    TheISA::MiscRegIndex reg_perm,
    TheISA::MiscRegIndex reg_rangesizetype,
    bool& out_faulted)
{
    out_faulted = false;
    Addr ret = 0;

    bool permitted = this->readMiscReg(reg_perm) == 1;
    if(!permitted) {
        out_faulted = true;
        return ret;
    }

    bool check_lower = this->readMiscReg(reg_rangesizetype) != 0;
    uint64_t new_base = this->readMiscReg(reg_base_address);
    uint64_t offset_limit = this->readMiscReg(reg_offset_limit);
    // TODO: add size of move to offset_limit
    uint32_t offset_limit_mid32 = (uint32_t) (offset_limit >> 16);
    uint32_t offset_limit_low32 = (uint32_t) offset_limit;
    uint32_t offset_limit_chosen32 = check_lower? offset_limit_low32 : offset_limit_mid32;

    uint64_t offset = scale * index + displacement;
    uint32_t offset_mid32 = (uint32_t) (offset >> 16);
    uint32_t offset_low32 = (uint32_t) offset;
    uint32_t offset_chosen32 = check_lower? offset_low32 : offset_mid32;

    // HFI with trusted sandboxes compares the chosen 32 bits
    bool hfi_is_inbounds = offset_chosen32 < offset_limit_chosen32;
    // We want this to simulate the actual bounds check
    // This works as long as the offset_limit is always a power of 2^16
    bool actual_is_inbounds = offset < offset_limit;

    // Sanity check
    if(hfi_is_inbounds != actual_is_inbounds) {
        printf("HFI: Unexpected oob\n");
        printHFIMetadata();
        abort();
    }

    if (!hfi_is_inbounds){
        out_faulted = true;
        return ret;
    }

    DPRINTF(HFI, "Replaced "
        "(r%" PRIu64 ", %" PRIu64 " * %" PRIu64 " + %" PRIu64 ")"
        " with "
        "(%" PRIu64 "+ %" PRIu64 " * %" PRIu64 " + %" PRIu64 ")"
        "\n",
        segment_index, scale, index, displacement,
        new_base, scale, index, displacement
    );

    ret = new_base + offset;
    return ret;
}

template <class Impl>
void BaseO3DynInst<Impl>::doHFIMaskCheck(Addr EA,
    TheISA::MiscRegIndex reg_base_mask,
    TheISA::MiscRegIndex reg_ignore_mask,
    TheISA::MiscRegIndex reg_perm,
    bool& out_found, bool& out_faulted)
{

    uint64_t addr = EA;
    uint64_t base_mask = this->readMiscReg(reg_base_mask);
    uint64_t ignore_mask = this->readMiscReg(reg_ignore_mask);

    // all of the bits except the ignore bits have to match
    out_found = (addr & ignore_mask) == base_mask;
    out_faulted = false;

    if (out_found) {
        bool permitted = this->readMiscReg(reg_perm) == 1;
        if (!permitted) {
            out_faulted = true;
        }
    }
}

template <class Impl>
Fault
BaseO3DynInst<Impl>::checkHFI(Addr &EA, bool is_store, uint64_t scale, uint64_t index, uint64_t base, uint64_t displacement){
    using namespace TheISA;

    bool is_inside_sandbox = this->readMiscReg(MISCREG_HFI_INSIDE_SANDBOX) != 0;

    if (!is_inside_sandbox) {
        return NoFault;
    }

    bool is_trusted_sandbox = this->readMiscReg(MISCREG_HFI_IS_TRUSTED_SANDBOX) == 1;
    bool is_hfi_structured_mov = this->macroop->isHFIStuctured();
    bool is_hfi_structured_mov1 = this->macroop->isHFIStuctured1();
    bool is_hfi_structured_mov2 = this->macroop->isHFIStuctured2();
    bool is_hfi_structured_mov3 = this->macroop->isHFIStuctured3();
    bool is_hfi_structured_mov4 = this->macroop->isHFIStuctured4();
    bool is_hfi_structured_mov_any = is_hfi_structured_mov ||
        is_hfi_structured_mov1 ||
        is_hfi_structured_mov2 ||
        is_hfi_structured_mov3 ||
        is_hfi_structured_mov4;

    MiscRegIndex hfi_regs_base[]          = { MISCREG_HFI_LINEAR_RANGE_1_BASE_ADDRESS_BASE_MASK  , MISCREG_HFI_LINEAR_RANGE_2_BASE_ADDRESS_BASE_MASK  , MISCREG_HFI_LINEAR_RANGE_3_BASE_ADDRESS_BASE_MASK  , MISCREG_HFI_LINEAR_RANGE_4_BASE_ADDRESS_BASE_MASK   };
    MiscRegIndex hfi_regs_offset_ignore[] = { MISCREG_HFI_LINEAR_RANGE_1_OFFSET_LIMIT_IGNORE_MASK, MISCREG_HFI_LINEAR_RANGE_2_OFFSET_LIMIT_IGNORE_MASK, MISCREG_HFI_LINEAR_RANGE_3_OFFSET_LIMIT_IGNORE_MASK, MISCREG_HFI_LINEAR_RANGE_4_OFFSET_LIMIT_IGNORE_MASK };
    MiscRegIndex hfi_regs_read[]          = { MISCREG_HFI_LINEAR_RANGE_1_READABLE                , MISCREG_HFI_LINEAR_RANGE_2_READABLE                , MISCREG_HFI_LINEAR_RANGE_3_READABLE                , MISCREG_HFI_LINEAR_RANGE_4_READABLE                 };
    MiscRegIndex hfi_regs_write[]         = { MISCREG_HFI_LINEAR_RANGE_1_WRITEABLE               , MISCREG_HFI_LINEAR_RANGE_2_WRITEABLE               , MISCREG_HFI_LINEAR_RANGE_3_WRITEABLE               , MISCREG_HFI_LINEAR_RANGE_4_WRITEABLE                };
    MiscRegIndex* hfi_regs_perm           = is_store? hfi_regs_write : hfi_regs_read;
    MiscRegIndex hfi_regs_rangesizetype[] = { MISCREG_HFI_LINEAR_RANGE_1_RANGESIZETYPE           , MISCREG_HFI_LINEAR_RANGE_2_RANGESIZETYPE           , MISCREG_HFI_LINEAR_RANGE_3_RANGESIZETYPE           , MISCREG_HFI_LINEAR_RANGE_4_RANGESIZETYPE            };

    if (is_trusted_sandbox) {

        if (is_hfi_structured_mov_any) {
            uint64_t segment_number = 0;

            if (is_hfi_structured_mov) {
                segment_number = base;
            } else {
                if (base != 0 && index != 0) {
                    printf("Used an hfi_structured_movN prefix with a non zero base\n");
                    abort();
                } else if (base != 0 && index == 0) {
                    // standardize on unused base for hfi_structured_mov_N instruction
                    index = base;
                    scale = 1;
                    base = 0;
                }
                // else if (base == 0 && index != 0) {
                // noop
                // }

                if (is_hfi_structured_mov1){ segment_number = 1; }
                else if (is_hfi_structured_mov2){ segment_number = 2; }
                else if (is_hfi_structured_mov3){ segment_number = 3; }
                else if (is_hfi_structured_mov4){ segment_number = 4; }
            }

            if (segment_number < 1 && segment_number > 4) {
                EA = 0;
                return std::make_shared<TheISA::HFIBoundsCheck>();
            }

            bool faulted = false;
            uint64_t segment_index = segment_number - 1;

            uint64_t newAddress = doHFIStructuredMov(segment_index, scale, index, displacement,
                hfi_regs_base[segment_index],
                hfi_regs_offset_ignore[segment_index],
                hfi_regs_perm[segment_index],
                hfi_regs_rangesizetype[segment_index],
                /* out */ faulted
            );

            if(faulted) {
                // printf("HFI hmov oob\n");
                printHFIMetadata();
                EA = 0;
                return std::make_shared<TheISA::HFIBoundsCheck>();
            }

            EA = newAddress;

        }
    } else {
        if (is_hfi_structured_mov) {
            return std::make_shared<TheISA::HFIIllegalInst>();
        }

        bool found = false;
        bool faulted = false;

        for (int i = 0; i < 4; i++) {
            doHFIMaskCheck(EA, hfi_regs_base[i], hfi_regs_offset_ignore[i], hfi_regs_perm[i], /* out */ found, /* out */ faulted);
            if (found) {
                break;
            }
        }

        if (faulted || !found) {
            // printf("HFI data mask oob\n");
            printHFIMetadata();
            EA = 0;
            return std::make_shared<TheISA::HFIBoundsCheck>();
        }
    }

    return NoFault;
}

template <class Impl>
void
BaseO3DynInst<Impl>::trap(const Fault &fault)
{
    this->cpu->trap(fault, this->threadNumber, this->staticInst);
}

template <class Impl>
void
BaseO3DynInst<Impl>::syscall()
{
    // HACK: check CPU's nextPC before and after syscall. If it
    // changes, update this instruction's nextPC because the syscall
    // must have changed the nextPC.
    TheISA::PCState curPC = this->cpu->pcState(this->threadNumber);
    this->cpu->syscall(this->threadNumber);
    TheISA::PCState newPC = this->cpu->pcState(this->threadNumber);
    if (!(curPC == newPC)) {
        this->pcState(newPC);
    }
}

#endif//__CPU_O3_DYN_INST_IMPL_HH__
