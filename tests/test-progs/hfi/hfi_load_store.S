.text

.globl hfi_load_store_test
.type hfi_load_store_test,@function
.align 16
hfi_load_store_test:
_hfi_load_store_test:
    # rdi: param 1 is the sandbox params
    # rsi: param 2 is the load address
    # rdx: param 3 is the store address
    # enter the sandbox, perform load&store, then exit
    mov %rdi, %rax
    .byte   0x0F, 0x04, 0x71, 0x00 # set sandbox metadata
    .byte   0x0F, 0x04, 0x65, 0x00 # enter sandbox
    mov 0x0(%rsi), %rcx
    mov %rcx, 0x0(%rdx)
    .byte   0x0F, 0x04, 0x66, 0x00 # exit sandbox
    ret

.globl hfi_load_store_ret_test
.type hfi_load_store_ret_test,@function
.align 16
hfi_load_store_ret_test:
_hfi_load_store_ret_test:
    # rdi: param 1 is the sandbox params
    # rsi: param 2 is the load address
    # rdx: param 3 is the store address
    # enter the sandbox, perform load&store, then exit
    mov %rdi, %rax
    .byte   0x0F, 0x04, 0x71, 0x00 # set sandbox metadata
    .byte   0x0F, 0x04, 0x65, 0x00 # enter sandbox
    mov 0x0(%rsi), %rcx
    mov %rcx, 0x0(%rdx)
    .byte   0x0F, 0x04, 0x66, 0x00 # exit sandbox
    ret

.globl hfi_load_test
.type hfi_load_test,@function
.align 16
hfi_load_test:
_hfi_load_test:
    # rdi: param 1 is the sandbox params
    # rsi: param 2 is the load address
    # enter the sandbox, perform load, then exit
    mov %rdi, %rax
    .byte   0x0F, 0x04, 0x71, 0x00 # set sandbox metadata
    .byte   0x0F, 0x04, 0x65, 0x00 # enter sandbox
    mov 0x0(%rsi), %rax
    .byte   0x0F, 0x04, 0x66, 0x00 # exit sandbox
    ret

.globl hfi_store_test
.type hfi_store_test,@function
.align 16
hfi_store_test:
_hfi_store_test:
    # rdi: param 1 is the sandbox params
    # rsi: param 2 is the store address
    # rdx: param 3 is the store value
    # enter the sandbox, perform store, then exit
    mov %rdi, %rax
    .byte   0x0F, 0x04, 0x71, 0x00 # set sandbox metadata
    .byte   0x0F, 0x04, 0x65, 0x00 # enter sandbox
    mov %rdx, 0x0(%rsi)
    .byte   0x0F, 0x04, 0x66, 0x00 # exit sandbox
    ret

.globl noop_func
.type noop_func,@function
.align 16
noop_func:
_noop_func:
    ret

.globl hfi_loadsavecontext_test
.type hfi_loadsavecontext_test,@function
.align 16
hfi_loadsavecontext_test:
_hfi_loadsavecontext_test:
    # rdi: param 1 is the sandbox params
    # rsi: param 2 is the thread_context to save into
    # rdx: param 3 is the thread_context to load from
    # rcx: param 4 is the thread_context to save into the second time
    # enter the sandbox, perform savecontext, loadcontext, savecontext then exit
    mov %rdi, %rax
    .byte   0x0F, 0x04, 0x71, 0x00 # set sandbox metadata
    .byte   0x0F, 0x04, 0x65, 0x00 # enter sandbox
    mov %rsi, %rax
    .byte   0x0F, 0x04, 0x67, 0x00 # save context
    mov %rdx, %rax
    .byte   0x0F, 0x04, 0x68, 0x00 # load context
    mov %rcx, %rax
    .byte   0x0F, 0x04, 0x67, 0x00 # save context
    .byte   0x0F, 0x04, 0x66, 0x00 # exit sandbox
    ret

.globl hfi_setgetmetadata_test
.type hfi_setgetmetadata_test,@function
.align 16
hfi_setgetmetadata_test:
_hfi_setgetmetadata_test:
    # rdi: param 1 is the sandbox metadata to save from
    # rsi: param 2 is the sandbox metadata to restore to
    mov %rdi, %rax
    .byte   0x0F, 0x04, 0x71, 0x00 # set sandbox metadata
    mov %rsi, %rax
    .byte   0x0F, 0x04, 0x72, 0x00 # get sandbox metadata
    ret

.globl hfi_exit_handler_test
.type hfi_exit_handler_test,@function
.align 16
hfi_exit_handler_test:
_hfi_exit_handler_test:
    # rdi: param 1 is the sandbox params
    # enter the sandbox, then exit. The exit would invoke the exit handler which will directly return
    mov %rdi, %rax
    .byte   0x0F, 0x04, 0x71, 0x00 # set sandbox metadata
    .byte   0x0F, 0x04, 0x65, 0x00 # enter sandbox

.globl hfi_test_exit_location
.type hfi_test_exit_location,@function
hfi_test_exit_location:
    .byte   0x0F, 0x04, 0x66, 0x00 # exit sandbox
    ud2
