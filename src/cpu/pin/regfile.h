#pragma once

#ifdef __cplusplus
# include <cstdint>
#else
# include <stdint.h>
#endif

struct PinRegFile
{
    // Integer register file.
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rsp, rbp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip;

    // Float register file.
    uint8_t fprs[8][10];
    uint64_t xmms[16][2];
    uint16_t fcw, fsw, ftag;

    // Misc register file.
    uint64_t rflags;
    uint16_t fs, gs;
    uint64_t fs_base, gs_base;
};
