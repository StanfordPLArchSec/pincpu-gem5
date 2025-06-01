#include <pin.H>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "plugin.hh"
#include "client.hh"

/**
 * NOTE: This implementation only supports untrusted sandboxes.
 */

namespace {

KNOB<bool> enable(KNOB_MODE_WRITEONCE, "pintool", "hfi", "0", "enable HFI support");

constexpr uint8_t HF1_PREFIX = 0x65;
constexpr uint8_t DUMMY_PREFIX = 0x36; // SS.

ADDRINT
pageof(ADDRINT addr)
{
    return addr & ~static_cast<ADDRINT>(0xFFF);
}

enum class HfiOp : uint8_t
{
    SetSandboxMetadata = 0x71,
    EnterSandbox = 0x65,
    ExitSandbox = 0x66,
};

std::unordered_map<ADDRINT, HfiOp> hfi_ops;
std::unordered_set<ADDRINT> hf1_ops;
std::unordered_map<ADDRINT, uint8_t> code_subst;
std::vector<REG> hfi_claimed_tool_regs;

static REG
getToolReg(unsigned i)
{
    while (i >= hfi_claimed_tool_regs.size())
        hfi_claimed_tool_regs.push_back(PIN_ClaimToolRegister());
    return hfi_claimed_tool_regs.at(i);
}

void
SetSandboxMetadata(ADDRINT base_)
{
    // TODO: Raise #GP fault instead of assertion failure.
    assert(!regfile.hfi.inside_sandbox || regfile.hfi.metadata.is_trusted_sandbox);
    const void *base = reinterpret_cast<const void *>(base_);
    void *dst = &regfile.hfi.metadata;
    const size_t bytes_to_copy = sizeof(HFIMetadata);
    const size_t bytes_copied = PIN_SafeCopy(dst, base, bytes_to_copy);
    assert(bytes_copied == bytes_to_copy);
}

void
EnterSandbox()
{
    // TODO: Raise #GP fault instead of assertion failure.
    assert(!regfile.hfi.inside_sandbox || regfile.hfi.metadata.is_trusted_sandbox);
    regfile.hfi.inside_sandbox = true;
    std::cerr << "[hfi] entering sandbox\n";
}

ADDRINT
ExitSandbox(ADDRINT pc, ADDRINT npc)
{
    // TODO: Raise #GP fault instead of assertion failure.
    assert(regfile.hfi.inside_sandbox);

    regfile.hfi.exit_reason = 1024;
    regfile.hfi.exit_location = pc;
    regfile.hfi.inside_sandbox = false;

    ADDRINT handler = regfile.hfi.metadata.exit_sandbox_handler;
    if (!handler)
        handler = npc;

    return handler;
}

ADDRINT
DoHFIStructuredMove(ADDRINT addr)
{
    const ADDRINT base = regfile.hfi.metadata.data_ranges[0].base_address_base_mask;
    return base + addr;
}

void
Stutter(const CONTEXT *ctx)
{
    PIN_RemoveInstrumentation();
    PIN_ExecuteAt(ctx);
}

void
InstrumentINS(INS ins, void *)
{
    if (IsKernelCode(ins))
        return;

    // Record and substitute out any HF1 prefixes, and then reinstrument.
    if (INS_SegmentPrefix(ins) && INS_SegmentRegPrefix(ins) == REG_SEG_GS) {
        std::vector<uint8_t> inst_bytes(INS_Size(ins));
        PIN_SafeCopy(inst_bytes.data(), reinterpret_cast<void *>(INS_Address(ins)), inst_bytes.size());
        const auto inst_bytes_it = std::find(inst_bytes.begin(), inst_bytes.end(), HF1_PREFIX);
        assert(inst_bytes_it != inst_bytes.end());
        [[maybe_unused]] const ADDRINT prefix_addr = INS_Address(ins) + (inst_bytes_it - inst_bytes.begin());
        code_subst[prefix_addr] = DUMMY_PREFIX; 
        hf1_ops.insert(INS_Address(ins));
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) Stutter,
                       IARG_CONST_CONTEXT,
                       IARG_END);
        return;
    }

    // Handle any HF1 accesses.
    if (hf1_ops.count(INS_Address(ins))) {
        // Rewrite the memory operand.
        for (UINT32 mop = 0; mop < INS_MemoryOperandCount(ins); ++mop) {
            const REG reg = (REG) getToolReg(mop);
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) DoHFIStructuredMove,
                           IARG_MEMORYOP_EA, mop,
                           IARG_RETURN_REGS, reg,
                           IARG_CALL_ORDER, CALL_ORDER_LAST, // TODO: Probably don't need this.
                           IARG_END);
            INS_RewriteMemoryOperand(ins, mop, reg);
        }
        return;
    }

    const auto hfi_op_it = hfi_ops.find(INS_Address(ins));
    if (hfi_op_it == hfi_ops.end())
        return;

    const HfiOp op = hfi_op_it->second;
    switch (op) {
      case HfiOp::SetSandboxMetadata:
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) SetSandboxMetadata,
                       IARG_REG_VALUE, REG_RAX,
                       IARG_END);
        break;

      case HfiOp::EnterSandbox:
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) EnterSandbox,
                       IARG_END);
        break;

      case HfiOp::ExitSandbox:
        {
            const REG handler = getToolReg(0);
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) ExitSandbox,
                           IARG_INST_PTR,
                           IARG_PTR, INS_Address(ins) + INS_Size(ins),
                           IARG_RETURN_REGS, handler,
                           IARG_END);
            INS_InsertIndirectJump(ins, IPOINT_BEFORE, handler);
            break;
        }

      default:
        std::cerr << "unhandled HFI opcode: " << std::hex << static_cast<unsigned>(op) << std::endl;
        std::abort();
    }
}

bool
SignalUD(THREADID tid, int32_t sig, CONTEXT *ctx, bool has_handler, const EXCEPTION_INFO *ex, void *)
{
    assert(sig == SIGILL);

    std::cerr << __FILE__ << ":" << __LINE__ << ": here\n";

    if (ex->GetExceptCode() != EXCEPTCODE_ILLEGAL_INS)
        return true;

    const ADDRINT pc = ex->GetExceptAddress();

    // Is this an HFI op?
    // TODO: This might trigger an error if it straddles two pages.
    // For now, just abort with error if we detect this condition.
    
    uint8_t opcode[4];
    if (pageof(pc) != pageof(pc + sizeof opcode - 1)) {
        std::cerr << "hfi: internal error: custom opcodes straddling pages unsupported" << std::endl;
        std::abort();
    }
    if (PIN_SafeCopy(opcode, reinterpret_cast<void *>(pc), sizeof opcode) != sizeof opcode) {
        std::cerr << "hfi: internal error: copy failed" << std::endl;
        std::abort();
    }

    // Is this an HFI op?
    if (!(opcode[0] == 0x0f && opcode[1] == 0x04))
        return true; // No, so pass on the signal.

    // This is an HFI op. Let's see which.
    assert(!hfi_ops.count(pc));
    const HfiOp op = static_cast<HfiOp>(opcode[2]);
    switch (op) {
      case HfiOp::SetSandboxMetadata:
      case HfiOp::EnterSandbox:
      case HfiOp::ExitSandbox:
        break;

      default:
        std::cerr << "hfi: unimplemented: subop 0x" << std::hex << static_cast<unsigned>(opcode[2]) << std::endl;
        std::cerr << "pc: " << pc << std::endl;
        std::abort();
    }

    hfi_ops[pc] = op;

    // Update the code substitution table.
    for (size_t i = 0; i < 4; ++i)
        code_subst[pc + i] = 0x90; // NOP

    // Clear the code cache.
    // TODO: Surgically clear it just for the necessary bytes.
    PIN_RemoveInstrumentation();

    // Don't pass on the signal, since we handled it.
    return false;
}

size_t
FetchCode(void *buf_, ADDRINT addr, size_t size, EXCEPTION_INFO *pExceptInfo, void *)
{
    uint8_t *buf = static_cast<uint8_t *>(buf_);

    const size_t bytes_copied = PIN_SafeCopyEx(buf, reinterpret_cast<void *>(addr), size, pExceptInfo);

    // Patch up.
    // TODO: This is horrifically inefficient. Make this more efficient.
    for (const auto &[sub_addr, newbyte] : code_subst)
        if (addr <= sub_addr && sub_addr < addr + bytes_copied)
            buf[sub_addr - addr] = newbyte;

    if (pExceptInfo->GetExceptCode() != EXCEPTCODE_NONE) {
        assert(bytes_copied != size);
        pExceptInfo->SetExceptAddress(reinterpret_cast<ADDRINT>(buf + bytes_copied));
        assert(pExceptInfo->GetExceptAddress() != 0);
    }

    return bytes_copied;
}

struct HFIPlugin final : Plugin
{
    const char *name() const override { return "hfi"; }

    int priority() const override { return -10; }

    bool enabled() const override { return enable.Value(); }

    bool
    reg() override
    {
        INS_AddInstrumentFunction(InstrumentINS, nullptr);
        PIN_InterceptSignal(SIGILL, SignalUD, nullptr);
        PIN_AddFetchFunction(FetchCode, nullptr);
        return true;
    }
} plugin;

}
