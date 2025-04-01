# Copyright (c) 2007 The Hewlett-Packard Development Company
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

microcode = """
def macroop ADD_R_R
{
    add reg, reg, regm, flags=(OF,SF,ZF,AF,PF,CF)
};

def macroop HFI_GET_VERSION
{
    limm rax, 2
};

def macroop HFI_GET_LINEAR_DATA_RANGE_COUNT
{
    limm rax, 4
};

def macroop HFI_GET_LINEAR_CODE_RANGE_COUNT
{
    limm rax, 2
};

def macroop HFI_SET_SANDBOX_METADATA
{
    # if (reg_hfi_curr.inside_sandbox && !reg_hfi_curr.is_trusted_sandbox) { SIGSEV; }
    rdval t1, ctrlRegIdx("misc_reg::HFI_INSIDE_SANDBOX")
    rdval t2, ctrlRegIdx("misc_reg::HFI_IS_TRUSTED_SANDBOX")
    # Get !t2
    xori t2, t2, 1
    and t1, t1, t2, flags=(ZF,)
    br label("hfi_set_sandbox_metadata_continue"), flags=(CZF,)
    fault "std::make_shared<HFIIllegalInst>()",

hfi_set_sandbox_metadata_continue:
    limm t1, 0
    limm t2, 0
    limm t3, 0
    limm t4, 0
    ld t2, seg, [1, t1, rax], dataSize=1
    ld t3, seg, [1, t1, rax], 1, dataSize=1
    ld t4, seg, [1, t1, rax], 2, dataSize=1
    ld t5, seg, [1, t1, rax], 8, dataSize=8
    ld t6, seg, [1, t1, rax], 16, dataSize=8
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_1_READABLE"), t2
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_1_WRITEABLE"), t3
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_1_RANGESIZETYPE"), t4
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_1_BASE_ADDRESS_BASE_MASK"), t5
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_1_OFFSET_LIMIT_IGNORE_MASK"), t6

    limm t1, 24
    limm t2, 0
    limm t3, 0
    limm t4, 0
    ld t2, seg, [1, t1, rax], dataSize=1
    ld t3, seg, [1, t1, rax], 1, dataSize=1
    ld t4, seg, [1, t1, rax], 2, dataSize=1
    ld t5, seg, [1, t1, rax], 8, dataSize=8
    ld t6, seg, [1, t1, rax], 16, dataSize=8
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_2_READABLE"), t2
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_2_WRITEABLE"), t3
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_2_RANGESIZETYPE"), t4
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_2_BASE_ADDRESS_BASE_MASK"), t5
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_2_OFFSET_LIMIT_IGNORE_MASK"), t6

    limm t1, 48
    limm t2, 0
    limm t3, 0
    limm t4, 0
    ld t2, seg, [1, t1, rax], dataSize=1
    ld t3, seg, [1, t1, rax], 1, dataSize=1
    ld t4, seg, [1, t1, rax], 2, dataSize=1
    ld t5, seg, [1, t1, rax], 8, dataSize=8
    ld t6, seg, [1, t1, rax], 16, dataSize=8
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_3_READABLE"), t2
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_3_WRITEABLE"), t3
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_3_RANGESIZETYPE"), t4
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_3_BASE_ADDRESS_BASE_MASK"), t5
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_3_OFFSET_LIMIT_IGNORE_MASK"), t6

    limm t1, 72
    limm t2, 0
    limm t3, 0
    limm t4, 0
    ld t2, seg, [1, t1, rax], dataSize=1
    ld t3, seg, [1, t1, rax], 1, dataSize=1
    ld t4, seg, [1, t1, rax], 2, dataSize=1
    ld t5, seg, [1, t1, rax], 8, dataSize=8
    ld t6, seg, [1, t1, rax], 16, dataSize=8
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_4_READABLE"), t2
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_4_WRITEABLE"), t3
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_4_RANGESIZETYPE"), t4
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_4_BASE_ADDRESS_BASE_MASK"), t5
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_4_OFFSET_LIMIT_IGNORE_MASK"), t6

    limm t1, 96
    limm t2, 0
    ld t2, seg, [1, t1, rax], dataSize=1
    ld t5, seg, [1, t1, rax], 8, dataSize=8
    ld t6, seg, [1, t1, rax], 16, dataSize=8
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_CODERANGE_1_EXECUTABLE"), t2
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_CODERANGE_1_BASE_MASK"), t5
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_CODERANGE_1_IGNORE_MASK"), t6

    limm t1, 120
    limm t2, 0
    ld t2, seg, [1, t1, rax], dataSize=1
    ld t5, seg, [1, t1, rax], 8, dataSize=8
    ld t6, seg, [1, t1, rax], 16, dataSize=8
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_CODERANGE_2_EXECUTABLE"), t2
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_CODERANGE_2_BASE_MASK"), t5
    wrval ctrlRegIdx("misc_reg::HFI_LINEAR_CODERANGE_2_IGNORE_MASK"), t6

    limm t1, 144
    limm t2, 0
    ld t2, seg, [1, t1, rax], dataSize=1
    ld t4, seg, [1, t1, rax], 8, dataSize=8
    wrval ctrlRegIdx("misc_reg::HFI_IS_TRUSTED_SANDBOX"), t2
    wrval ctrlRegIdx("misc_reg::HFI_EXIT_SANDBOX_HANDLER"), t4
};

def macroop HFI_GET_SANDBOX_METADATA
{
    # if (reg_hfi_curr.inside_sandbox && !reg_hfi_curr.is_trusted_sandbox) { SIGSEV; }
    rdval t1, ctrlRegIdx("misc_reg::HFI_INSIDE_SANDBOX")
    rdval t2, ctrlRegIdx("misc_reg::HFI_IS_TRUSTED_SANDBOX")
    # Get !t2
    xori t2, t2, 1
    and t1, t1, t2, flags=(ZF,)
    br label("hfi_get_sandbox_metadata_continue"), flags=(CZF,)
    fault "std::make_shared<HFIIllegalInst>()",

hfi_get_sandbox_metadata_continue:
    limm t1, 0
    rdval t2, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_1_READABLE")
    rdval t3, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_1_WRITEABLE")
    rdval t4, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_1_RANGESIZETYPE")
    rdval t5, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_1_BASE_ADDRESS_BASE_MASK")
    rdval t6, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_1_OFFSET_LIMIT_IGNORE_MASK")
    st t2, seg, [1, t1, rax], dataSize=1
    st t3, seg, [1, t1, rax], 1, dataSize=1
    st t4, seg, [1, t1, rax], 2, dataSize=1
    st t5, seg, [1, t1, rax], 8, dataSize=8
    st t6, seg, [1, t1, rax], 16, dataSize=8

    limm t1, 24
    rdval t2, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_2_READABLE")
    rdval t3, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_2_WRITEABLE")
    rdval t4, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_2_RANGESIZETYPE")
    rdval t5, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_2_BASE_ADDRESS_BASE_MASK")
    rdval t6, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_2_OFFSET_LIMIT_IGNORE_MASK")
    st t2, seg, [1, t1, rax], dataSize=1
    st t3, seg, [1, t1, rax], 1, dataSize=1
    st t4, seg, [1, t1, rax], 2, dataSize=1
    st t5, seg, [1, t1, rax], 8, dataSize=8
    st t6, seg, [1, t1, rax], 16, dataSize=8

    limm t1, 48
    rdval t2, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_3_READABLE")
    rdval t3, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_3_WRITEABLE")
    rdval t4, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_3_RANGESIZETYPE")
    rdval t5, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_3_BASE_ADDRESS_BASE_MASK")
    rdval t6, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_3_OFFSET_LIMIT_IGNORE_MASK")
    st t2, seg, [1, t1, rax], dataSize=1
    st t3, seg, [1, t1, rax], 1, dataSize=1
    st t4, seg, [1, t1, rax], 2, dataSize=1
    st t5, seg, [1, t1, rax], 8, dataSize=8
    st t6, seg, [1, t1, rax], 16, dataSize=8

    limm t1, 72
    rdval t2, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_4_READABLE")
    rdval t3, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_4_WRITEABLE")
    rdval t4, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_4_RANGESIZETYPE")
    rdval t5, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_4_BASE_ADDRESS_BASE_MASK")
    rdval t6, ctrlRegIdx("misc_reg::HFI_LINEAR_RANGE_4_OFFSET_LIMIT_IGNORE_MASK")
    st t2, seg, [1, t1, rax], dataSize=1
    st t3, seg, [1, t1, rax], 1, dataSize=1
    st t4, seg, [1, t1, rax], 2, dataSize=1
    st t5, seg, [1, t1, rax], 8, dataSize=8
    st t6, seg, [1, t1, rax], 16, dataSize=8

    limm t1, 96
    rdval t2, ctrlRegIdx("misc_reg::HFI_LINEAR_CODERANGE_1_EXECUTABLE")
    rdval t5, ctrlRegIdx("misc_reg::HFI_LINEAR_CODERANGE_1_BASE_MASK")
    rdval t6, ctrlRegIdx("misc_reg::HFI_LINEAR_CODERANGE_1_IGNORE_MASK")
    st t2, seg, [1, t1, rax], dataSize=1
    st t5, seg, [1, t1, rax], 8, dataSize=8
    st t6, seg, [1, t1, rax], 16, dataSize=8

    limm t1, 120
    rdval t2, ctrlRegIdx("misc_reg::HFI_LINEAR_CODERANGE_2_EXECUTABLE")
    rdval t5, ctrlRegIdx("misc_reg::HFI_LINEAR_CODERANGE_2_BASE_MASK")
    rdval t6, ctrlRegIdx("misc_reg::HFI_LINEAR_CODERANGE_2_IGNORE_MASK")
    st t2, seg, [1, t1, rax], dataSize=1
    st t5, seg, [1, t1, rax], 8, dataSize=8
    st t6, seg, [1, t1, rax], 16, dataSize=8

    limm t1, 144
    rdval t2, ctrlRegIdx("misc_reg::HFI_IS_TRUSTED_SANDBOX")
    rdval t4, ctrlRegIdx("misc_reg::HFI_EXIT_SANDBOX_HANDLER")
    st t2, seg, [1, t1, rax], dataSize=1
    st t4, seg, [1, t1, rax], 8, dataSize=8
};

def macroop HFI_ENTER_SANDBOX
{
    # if (reg_hfi_curr.inside_sandbox && !reg_hfi_curr.is_trusted_sandbox) { SIGSEV; }
    rdval t1, ctrlRegIdx("misc_reg::HFI_INSIDE_SANDBOX")
    rdval t2, ctrlRegIdx("misc_reg::HFI_IS_TRUSTED_SANDBOX")
    # Get !t2
    xori t2, t2, 1
    and t1, t1, t2, flags=(ZF,)
    br label("hfi_enter_sandbox_continue"), flags=(CZF,)
    fault "std::make_shared<HFIIllegalInst>()",

hfi_enter_sandbox_continue:
    limm t1, 1
    .serialize_after
    wrval ctrlRegIdx("misc_reg::HFI_INSIDE_SANDBOX"), t1
};

def macroop HFI_EXIT_SANDBOX
{
    # if (!reg_hfi_curr.inside_sandbox) { SIGSEV; }
    rdval t1, ctrlRegIdx("misc_reg::HFI_INSIDE_SANDBOX")
    and t1, t1, t1, flags=(ZF,)
    br label("hfi_exit_sandbox_continue"), flags=(nCZF,)
    fault "std::make_shared<HFIIllegalInst>()",

hfi_exit_sandbox_continue:
    limm t2, 1024
    wrval ctrlRegIdx("misc_reg::HFI_EXIT_REASON"), t2
    rdipc t3
    wrval ctrlRegIdx("misc_reg::HFI_EXIT_LOCATION"), t3
    wrval ctrlRegIdx("misc_reg::HFI_INSIDE_SANDBOX"), t0

    # if (exit_sandbox_handler) { jmp exit_sandbox_handler; }
    rdval t2, ctrlRegIdx("misc_reg::HFI_EXIT_SANDBOX_HANDLER")
    and t3, t2, t2, flags=(ZF,)
    .serialize_after
    br label("hfi_exit_sandbox_nohandler"), flags=(CZF,)

    # Make the default data size of jumps 64 bits in 64 bit mode
    .adjust_env oszIn64Override
    .control_indirect
    wripi t2, 0

hfi_exit_sandbox_nohandler:
    limm t0, 0
};

def macroop HFI_GET_EXIT_REASON
{
    # if (!reg_hfi_curr.inside_sandbox) { SIGSEV; }
    rdval t1, ctrlRegIdx("misc_reg::HFI_INSIDE_SANDBOX")
    and t1, t1, t1, flags=(ZF,)
    br label("hfi_get_exit_reason_continue"), flags=(nCZF,)
    fault "std::make_shared<HFIIllegalInst>()",

hfi_get_exit_reason_continue:
    rdval rax, ctrlRegIdx("misc_reg::HFI_EXIT_REASON")
};

def macroop HFI_GET_EXIT_LOCATION
{
    # if (!reg_hfi_curr.inside_sandbox) { SIGSEV; }
    rdval t1, ctrlRegIdx("misc_reg::HFI_INSIDE_SANDBOX")
    and t1, t1, t1, flags=(ZF,)
    br label("hfi_get_exit_location_continue"), flags=(nCZF,)
    fault "std::make_shared<HFIIllegalInst>()",

hfi_get_exit_location_continue:
    rdval rax, ctrlRegIdx("misc_reg::HFI_EXIT_LOCATION")
};

def macroop ADD_R_I
{
    limm t1, imm
    add reg, reg, t1, flags=(OF,SF,ZF,AF,PF,CF)
};

def macroop ADD_M_I
{
    limm t2, imm
    ldst t1, seg, sib, disp
    add t1, t1, t2, flags=(OF,SF,ZF,AF,PF,CF)
    st t1, seg, sib, disp
};

def macroop ADD_P_I
{
    rdip t7
    limm t2, imm
    ldst t1, seg, riprel, disp
    add t1, t1, t2, flags=(OF,SF,ZF,AF,PF,CF)
    st t1, seg, riprel, disp
};

def macroop ADD_LOCKED_M_I
{
    limm t2, imm
    mfence
    ldstl t1, seg, sib, disp
    add t1, t1, t2, flags=(OF,SF,ZF,AF,PF,CF)
    stul t1, seg, sib, disp
    mfence
};

def macroop ADD_LOCKED_P_I
{
    rdip t7
    limm t2, imm
    mfence
    ldstl t1, seg, riprel, disp
    add t1, t1, t2, flags=(OF,SF,ZF,AF,PF,CF)
    stul t1, seg, riprel, disp
    mfence
};

def macroop ADD_M_R
{
    ldst t1, seg, sib, disp
    add t1, t1, reg, flags=(OF,SF,ZF,AF,PF,CF)
    st t1, seg, sib, disp
};

def macroop ADD_P_R
{
    rdip t7
    ldst t1, seg, riprel, disp
    add t1, t1, reg, flags=(OF,SF,ZF,AF,PF,CF)
    st t1, seg, riprel, disp
};

def macroop ADD_LOCKED_M_R
{
    mfence
    ldstl t1, seg, sib, disp
    add t1, t1, reg, flags=(OF,SF,ZF,AF,PF,CF)
    stul t1, seg, sib, disp
    mfence
};

def macroop ADD_LOCKED_P_R
{
    rdip t7
    mfence
    ldstl t1, seg, riprel, disp
    add t1, t1, reg, flags=(OF,SF,ZF,AF,PF,CF)
    stul t1, seg, riprel, disp
    mfence
};

def macroop ADD_R_M
{
    ld t1, seg, sib, disp
    add reg, reg, t1, flags=(OF,SF,ZF,AF,PF,CF)
};

def macroop ADD_R_P
{
    rdip t7
    ld t1, seg, riprel, disp
    add reg, reg, t1, flags=(OF,SF,ZF,AF,PF,CF)
};

def macroop SUB_R_R
{
    sub reg, reg, regm, flags=(OF,SF,ZF,AF,PF,CF)
};

def macroop SUB_R_I
{
    limm t1, imm
    sub reg, reg, t1, flags=(OF,SF,ZF,AF,PF,CF)
};

def macroop SUB_R_M
{
    ld t1, seg, sib, disp
    sub reg, reg, t1, flags=(OF,SF,ZF,AF,PF,CF)
};

def macroop SUB_R_P
{
    rdip t7
    ld t1, seg, riprel, disp
    sub reg, reg, t1, flags=(OF,SF,ZF,AF,PF,CF)
};

def macroop SUB_M_I
{
    limm t2, imm
    ldst t1, seg, sib, disp
    sub t1, t1, t2, flags=(OF,SF,ZF,AF,PF,CF)
    st t1, seg, sib, disp
};

def macroop SUB_P_I
{
    rdip t7
    limm t2, imm
    ldst t1, seg, riprel, disp
    sub t1, t1, t2, flags=(OF,SF,ZF,AF,PF,CF)
    st t1, seg, riprel, disp
};

def macroop SUB_LOCKED_M_I
{
    limm t2, imm
    mfence
    ldstl t1, seg, sib, disp
    sub t1, t1, t2, flags=(OF,SF,ZF,AF,PF,CF)
    stul t1, seg, sib, disp
    mfence
};

def macroop SUB_LOCKED_P_I
{
    rdip t7
    limm t2, imm
    mfence
    ldstl t1, seg, riprel, disp
    sub t1, t1, t2, flags=(OF,SF,ZF,AF,PF,CF)
    stul t1, seg, riprel, disp
    mfence
};

def macroop SUB_M_R
{
    ldst t1, seg, sib, disp
    sub t1, t1, reg, flags=(OF,SF,ZF,AF,PF,CF)
    st t1, seg, sib, disp
};

def macroop SUB_P_R
{
    rdip t7
    ldst t1, seg, riprel, disp
    sub t1, t1, reg, flags=(OF,SF,ZF,AF,PF,CF)
    st t1, seg, riprel, disp
};

def macroop SUB_LOCKED_M_R
{
    mfence
    ldstl t1, seg, sib, disp
    sub t1, t1, reg, flags=(OF,SF,ZF,AF,PF,CF)
    stul t1, seg, sib, disp
    mfence
};

def macroop SUB_LOCKED_P_R
{
    rdip t7
    mfence
    ldstl t1, seg, riprel, disp
    sub t1, t1, reg, flags=(OF,SF,ZF,AF,PF,CF)
    stul t1, seg, riprel, disp
    mfence
};

def macroop ADC_R_R
{
    adc reg, reg, regm, flags=(OF,SF,ZF,AF,PF,CF)
};

def macroop ADC_R_I
{
    limm t1, imm
    adc reg, reg, t1, flags=(OF,SF,ZF,AF,PF,CF)
};

def macroop ADC_M_I
{
    limm t2, imm
    ldst t1, seg, sib, disp
    adc t1, t1, t2, flags=(OF,SF,ZF,AF,PF,CF)
    st t1, seg, sib, disp
};

def macroop ADC_P_I
{
    rdip t7
    limm t2, imm
    ldst t1, seg, riprel, disp
    adc t1, t1, t2, flags=(OF,SF,ZF,AF,PF,CF)
    st t1, seg, riprel, disp
};

def macroop ADC_LOCKED_M_I
{
    limm t2, imm
    mfence
    ldstl t1, seg, sib, disp
    adc t1, t1, t2, flags=(OF,SF,ZF,AF,PF,CF)
    stul t1, seg, sib, disp
    mfence
};

def macroop ADC_LOCKED_P_I
{
    rdip t7
    limm t2, imm
    mfence
    ldstl t1, seg, riprel, disp
    adc t1, t1, t2, flags=(OF,SF,ZF,AF,PF,CF)
    stul t1, seg, riprel, disp
    mfence
};

def macroop ADC_M_R
{
    ldst t1, seg, sib, disp
    adc t1, t1, reg, flags=(OF,SF,ZF,AF,PF,CF)
    st t1, seg, sib, disp
};

def macroop ADC_P_R
{
    rdip t7
    ldst t1, seg, riprel, disp
    adc t1, t1, reg, flags=(OF,SF,ZF,AF,PF,CF)
    st t1, seg, riprel, disp
};

def macroop ADC_LOCKED_M_R
{
    mfence
    ldstl t1, seg, sib, disp
    adc t1, t1, reg, flags=(OF,SF,ZF,AF,PF,CF)
    stul t1, seg, sib, disp
    mfence
};

def macroop ADC_LOCKED_P_R
{
    rdip t7
    mfence
    ldstl t1, seg, riprel, disp
    adc t1, t1, reg, flags=(OF,SF,ZF,AF,PF,CF)
    stul t1, seg, riprel, disp
    mfence
};

def macroop ADC_R_M
{
    ld t1, seg, sib, disp
    adc reg, reg, t1, flags=(OF,SF,ZF,AF,PF,CF)
};

def macroop ADC_R_P
{
    rdip t7
    ld t1, seg, riprel, disp
    adc reg, reg, t1, flags=(OF,SF,ZF,AF,PF,CF)
};

def macroop SBB_R_R
{
    sbb reg, reg, regm, flags=(OF,SF,ZF,AF,PF,CF)
};

def macroop SBB_R_I
{
    limm t1, imm
    sbb reg, reg, t1, flags=(OF,SF,ZF,AF,PF,CF)
};

def macroop SBB_R_M
{
    ld t1, seg, sib, disp
    sbb reg, reg, t1, flags=(OF,SF,ZF,AF,PF,CF)
};

def macroop SBB_R_P
{
    rdip t7
    ld t1, seg, riprel, disp
    sbb reg, reg, t1, flags=(OF,SF,ZF,AF,PF,CF)
};

def macroop SBB_M_I
{
    limm t2, imm
    ldst t1, seg, sib, disp
    sbb t1, t1, t2, flags=(OF,SF,ZF,AF,PF,CF)
    st t1, seg, sib, disp
};

def macroop SBB_P_I
{
    rdip t7
    limm t2, imm
    ldst t1, seg, riprel, disp
    sbb t1, t1, t2, flags=(OF,SF,ZF,AF,PF,CF)
    st t1, seg, riprel, disp
};

def macroop SBB_LOCKED_M_I
{
    limm t2, imm
    mfence
    ldstl t1, seg, sib, disp
    sbb t1, t1, t2, flags=(OF,SF,ZF,AF,PF,CF)
    stul t1, seg, sib, disp
    mfence
};

def macroop SBB_LOCKED_P_I
{
    rdip t7
    limm t2, imm
    mfence
    ldstl t1, seg, riprel, disp
    sbb t1, t1, t2, flags=(OF,SF,ZF,AF,PF,CF)
    stul t1, seg, riprel, disp
    mfence
};

def macroop SBB_M_R
{
    ldst t1, seg, sib, disp
    sbb t1, t1, reg, flags=(OF,SF,ZF,AF,PF,CF)
    st t1, seg, sib, disp
};

def macroop SBB_P_R
{
    rdip t7
    ldst t1, seg, riprel, disp
    sbb t1, t1, reg, flags=(OF,SF,ZF,AF,PF,CF)
    st t1, seg, riprel, disp
};

def macroop SBB_LOCKED_M_R
{
    mfence
    ldstl t1, seg, sib, disp
    sbb t1, t1, reg, flags=(OF,SF,ZF,AF,PF,CF)
    stul t1, seg, sib, disp
    mfence
};

def macroop SBB_LOCKED_P_R
{
    rdip t7
    mfence
    ldstl t1, seg, riprel, disp
    sbb t1, t1, reg, flags=(OF,SF,ZF,AF,PF,CF)
    stul t1, seg, riprel, disp
    mfence
};

def macroop NEG_R
{
    sub reg, t0, reg, flags=(CF,OF,SF,ZF,AF,PF)
};

def macroop NEG_M
{
    ldst t1, seg, sib, disp
    sub t1, t0, t1, flags=(CF,OF,SF,ZF,AF,PF)
    st t1, seg, sib, disp
};

def macroop NEG_P
{
    rdip t7
    ldst t1, seg, riprel, disp
    sub t1, t0, t1, flags=(CF,OF,SF,ZF,AF,PF)
    st t1, seg, riprel, disp
};

def macroop NEG_LOCKED_M
{
    mfence
    ldstl t1, seg, sib, disp
    sub t1, t0, t1, flags=(CF,OF,SF,ZF,AF,PF)
    stul t1, seg, sib, disp
    mfence
};

def macroop NEG_LOCKED_P
{
    rdip t7
    mfence
    ldstl t1, seg, riprel, disp
    sub t1, t0, t1, flags=(CF,OF,SF,ZF,AF,PF)
    stul t1, seg, riprel, disp
    mfence
};
"""
