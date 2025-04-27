#include "ops.hh"
#include "cpu/pin/regfile.h"

// TODO: macro for defining these, since they are all basically the same.

void __attribute__((naked)) pinop_set_reg(const char *name, const uint8_t *data, size_t size) {
    asm volatile ("movb $0, (%0)\nret\n"
		  :: "r"(pinops_addr_base + OP_SET_REG));
}

void __attribute__((naked)) pinop_get_reg(const char *name, uint8_t *data, size_t size) {
    asm volatile ("movb $0, (%0)\nret\n"
		  :: "r"(pinops_addr_base + OP_GET_REG));
}

void __attribute__((naked)) pinop_get_reqpath(char *data, size_t size) {
    asm volatile ("movb $0, (%0)\nret\n" :: "r"(pinops_addr_base + OP_GET_REQPATH));
}

void __attribute__((naked)) pinop_get_resppath(char *data, size_t size) {
    asm volatile ("movb $0, (%0)\nret\n" :: "r"(pinops_addr_base + OP_GET_RESPPATH));
}

void __attribute__((naked)) pinop_get_mempath(char *data, size_t size) {
    asm volatile ("movb $0, (%0)\nret\n" :: "r"(pinops_addr_base + OP_GET_MEMPATH));
}

void __attribute__((naked)) pinop_exit(int code) {
    asm volatile ("movb $0, (%0)\nret\n" :: "r"(pinops_addr_base + OP_EXIT));
}

void __attribute__((naked)) (pinop_abort)(const char *msg, size_t line) {
    asm volatile ("movq (%%rsp), %%rdx\n"
                  "movb $0, (%0)\nret\n" :: "r"(pinops_addr_base + OP_ABORT));
}

void __attribute__((naked)) pinop_resetuser() {
    asm volatile ("movb $0, (%0)\nret\n" :: "r"(pinops_addr_base + OP_RESETUSER));
}

void __attribute__((naked)) pinop_run(struct RunResult *result) {
    asm volatile ("movb $0, (%0)\nret\n" :: "r"(pinops_addr_base + OP_RUN));
}

void __attribute__((naked)) pinop_set_vsyscall_base(void *virt, void *phys) {
    asm volatile ("movb $0, (%0)\nret\n" :: "r"(pinops_addr_base + OP_SET_VSYSCALL_BASE));
}

uint64_t __attribute__((naked)) pinop_get_instcount(void) {
    asm volatile ("movb $0, (%0)\nret\n" :: "r"(pinops_addr_base + OP_GET_INSTCOUNT));
}

void __attribute__((naked)) pinop_set_regs(const struct PinRegFile *regfile) {
    asm volatile ("movb $0, (%0)\nret\n" :: "r"(pinops_addr_base + OP_SET_REGS));
}

void __attribute__((naked)) pinop_get_regs(struct PinRegFile *regfile) {
    asm volatile ("movb $0, (%0)\nret\n" :: "r"(pinops_addr_base + OP_GET_REGS));
}

void __attribute__((naked)) pinop_add_symbol(const char *name, void *vaddr) {
    asm volatile ("movb $0, (%0)\nret\n" :: "r"(pinops_addr_base + OP_ADD_SYMBOL));
}

size_t __attribute__((naked)) pinop_exec_command(const char *cmd) {
    asm volatile ("movb $0, (%0)\nret\n" :: "r"(pinops_addr_base + OP_EXEC_COMMAND));
}

void __attribute__((naked)) pinop_read_command_result(char *buf, size_t idx, size_t size) {
    asm volatile ("movb $0, (%0)\nret\n" :: "r"(pinops_addr_base + OP_READ_COMMAND_RESULT));
}
