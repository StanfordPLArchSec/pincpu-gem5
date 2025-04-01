#pragma once

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "hfi.h"

#ifdef __cplusplus
extern "C" {
#endif

inline hfi_sandbox get_full_access_mask_sandbox() {
    hfi_sandbox sandbox;
    memset(&sandbox, 0, sizeof(hfi_sandbox));

    sandbox.is_trusted_sandbox = false;
    sandbox.data_ranges[0].readable = true;
    sandbox.data_ranges[0].writeable = true;
    sandbox.data_ranges[0].base_mask = 0;
    sandbox.data_ranges[0].ignore_mask = (uint64_t)0xFFFFFFFFFFFFFFFF;

    sandbox.code_ranges[0].executable = true;
    sandbox.code_ranges[0].base_mask = 0;
    sandbox.code_ranges[0].ignore_mask = (uint64_t)0xFFFFFFFFFFFFFFFF;

    return sandbox;
}

void* assert_memcmp(const void* p1, const void* p2, size_t n) {
    bool is_equal = memcmp(p1, p2, n) == 0;
    if (!is_equal) {
        const char* c1 = (const char*) p1;
        const char* c2 = (const char*) p2;
        for(size_t i = 0; i < n; i++) {
            const char* diff_string = c1[i] == c2[i]? "" : "<----";
            printf("Byte %zu: (left, right): %02X, %02X %s\n", i, c1[i], c2[i], diff_string); fflush(stdout);
        }
        assert(is_equal);
    }
}

#ifdef __cplusplus
}
#endif
