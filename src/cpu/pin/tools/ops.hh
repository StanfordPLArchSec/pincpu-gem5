#pragma once

// TODO: This should just be ops.h, not ops.hh.

#ifdef __cplusplus
# include <cstdint>
#else
# include <stdint.h>
# include <stdbool.h>
# include <stddef.h>
#endif

enum PinOp
{
    OP_SET_REG,
    OP_GET_REQPATH,
    OP_GET_RESPPATH,
    OP_GET_MEMPATH,
    OP_ABORT,
    OP_EXIT,
    OP_RUN,
    OP_RESETUSER,
    OP_GET_REG,
    OP_SET_VSYSCALL_BASE,
    OP_GET_INSTCOUNT,
    OP_SET_REGS,
    OP_GET_REGS,
    OP_ADD_SYMBOL,
    OP_EXEC_COMMAND,
    OP_READ_COMMAND_RESULT,
    OP_COUNT,
};

#define pinops_addr_base ((uint64_t) 0xbaddecaf << 12)
#define pinops_addr_end (pinops_addr_base + OP_COUNT)

static inline bool is_pinop_addr(void *p)
{
    const uintptr_t s = (uintptr_t) p;
    return pinops_addr_base <= s && s < pinops_addr_end;
}

// TODO: Need utility functions for setting this from pintool.
struct RunResult {
    enum RunResultType {
        RUNRESULT_PAGEFAULT,
        RUNRESULT_SYSCALL,
        RUNRESULT_CPUID,
        RUNRESULT_BREAK,
	RUNRESULT_INSTCOUNT,
    } result;
    union {
        uint64_t addr; // RUNRESULT_PAGEFAULT
    };
};

// TODO: Only declare these in kernel, not pintool.
void pinop_set_reg(const char *name, const uint8_t *data, size_t size);
void pinop_get_reg(const char *name, uint8_t *data, size_t size);
void pinop_get_reqpath(char *data, size_t size);
void pinop_get_resppath(char *data, size_t size);
void pinop_get_mempath(char *data, size_t size);
void pinop_exit(int code);
void pinop_abort(const char *msg, size_t line);
void pinop_resetuser(void);
void pinop_run(struct RunResult *result);
void pinop_set_vsyscall_base(void *virt, void *phys);
uint64_t pinop_get_instcount(void);

struct PinRegFile;
void pinop_set_regs(const struct PinRegFile *regfile);
void pinop_get_regs(struct PinRegFile *regfile);

void pinop_add_symbol(const char *name, void *vaddr);

/// Returns the number of bytes in the command result.
size_t pinop_exec_command(const char *s);
void pinop_read_command_result(char *buf, size_t idx, size_t size);

#define pinop_abort() (pinop_abort)(__FILE__, __LINE__)
