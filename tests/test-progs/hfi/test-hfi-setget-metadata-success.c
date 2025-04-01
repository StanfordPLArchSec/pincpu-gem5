#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "test-common.h"
#include "hfi.h"

void hfi_setgetmetadata_test(hfi_sandbox* sandbox_to_save, hfi_sandbox* restored_sandbox);

int main(int argc, char* argv[]) {
    // This test sets the current metadata, loads it and checks if it is the same
    hfi_sandbox metadatas[2];
    memset(metadatas, 0, sizeof(hfi_sandbox) * 2);

    hfi_sandbox* sandbox_to_save  = &(metadatas[0]);
    hfi_sandbox* restored_sandbox  = &(metadatas[1]);

    // Fill random values
    sandbox_to_save->exit_sandbox_handler = (void*) 0x12344;
    sandbox_to_save->is_trusted_sandbox = true;

    for (int i = 0; i < HFI_LINEAR_DATA_RANGE_COUNT; i++) {
        sandbox_to_save->data_ranges[i].readable = (uint8_t) (i % 2);
        sandbox_to_save->data_ranges[i].writeable = (uint8_t) ((i + 1) % 2);
        sandbox_to_save->data_ranges[i].range_size_type = (uint8_t) ((i + 1) % 2);
        sandbox_to_save->data_ranges[i].base_mask = (uint64_t) 0x453450000000 * i;
        sandbox_to_save->data_ranges[i].ignore_mask = (uintptr_t) 0x345500000 * i;
    }

    for (int i = 0; i < HFI_LINEAR_CODE_RANGE_COUNT; i++) {
        sandbox_to_save->code_ranges[i].executable = (uint8_t) ((i + 1) % 2);
        sandbox_to_save->code_ranges[i].base_mask = (uint64_t) 0x764450000000 * i;
        sandbox_to_save->code_ranges[i].ignore_mask = (uintptr_t) 0x432500000 * i;
    }

    hfi_setgetmetadata_test(sandbox_to_save, restored_sandbox);

    assert_memcmp(sandbox_to_save, restored_sandbox, sizeof(hfi_sandbox));
    printf("Success\n");
}
