#pragma once

#ifdef __cplusplus
# include <cstdint>
#else
# include <stdint.h>
# include <stdbool.h>
# include <assert.h>
#endif

#define HFI_NUM_DATA_RANGES 4
#define HFI_NUM_CODE_RANGES 2

struct HFIDataRange
{
    uint8_t readable;
    uint8_t writeable;
    uint8_t rangesizetype;
    uint64_t base_address_base_mask;
    uint64_t offset_limit_ignore_mask;
};
static_assert(sizeof(struct HFIDataRange) == 24);

struct HFICodeRange
{
    uint8_t executable;
    uint64_t base_mask;
    uint64_t ignore_mask;
};
static_assert(sizeof(struct HFICodeRange) == 24);

struct HFIMetadata
{
    struct HFIDataRange data_ranges[4];
    struct HFICodeRange code_ranges[2];
    uint8_t is_trusted_sandbox;
    uint64_t exit_sandbox_handler;
};

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

    // HFI register file
    struct {
        bool inside_sandbox;
        uint64_t exit_reason;
        uint64_t exit_location;
        struct HFIMetadata metadata;
    } hfi;
};
