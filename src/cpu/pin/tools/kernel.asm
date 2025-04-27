bits 64
global _start

%define PINOP (0xbaddecaf << 32)
%define OP_SET_REG     0
%define OP_GET_CPUPATH 1
%define OP_GET_MEMPATH 2
%define OP_ABORT       3
%define OP_EXIT        4

%define SYS_read 0
%define SYS_write 1
%define SYS_open 2
%define SYS_close 3
%define SYS_mmap 9
%define SYS_munmap 11

%define SIZEOF_MESSAGE (128 + 4)

%define O_RDWR 2

	; Puts the result in RAX.
%define INVOKE_PINOP(cmd) mov byte [rbx + cmd], 0

	; void get_cpupath(void *data, size_t size);
	; void get_mempath(void *data, size_t size);

section .text
_start:
	; %rbx will always hold the PINOP base.
	mov rbx, PINOP

	; Stack frame
	sub rsp, 256

	; Get the CPU communication file path.
	mov rsi, 256
	lea rdi, [rsp + 0]
	INVOKE_PINOP(OP_GET_CPUPATH)

	; Open CPU comm file.
	mov eax, SYS_open
	lea rdi, [rsp + 0]
	mov esi, O_RDWR
	syscall
	cmp eax, 0
	jl abort
	mov [rel cpu_fd], eax

	; Open physmem.
	mov rsi, 256
	lea rdi, [rsp + 0]
	INVOKE_PINOP(OP_GET_MEMPATH)
	mov eax, SYS_open
	mov esi, O_RDWR
	syscall
	cmp eax, 0
	jl abort
	mov [rel mem_fd], eax
	jmp main_event_loop

	; void msg_read(void);
msg_read:
	mov eax, SYS_read
	mov edi, [rel cpu_fd]
	lea rsi, [rel message]
	mov edx, SIZEOF_MESSAGE
	syscall
	cmp eax, SIZEOF_MESSAGE
	jne abort
	ret

	; void msg_read(void);
msg_write:
	mov eax, SYS_write
	mov edi, [rel cpu_fd]
	lea rsi, [rel message]
	mov edx, SIZEOF_MESSAGE
	syscall
	cmp eax, SIZEOF_MESSAGE
	jne abort
	ret
	
main_event_loop:
	; read message
	call msg_read

	; switch on m

	jmp main_event_loop

exit:
	; exit for now
	INVOKE_PINOP(OP_EXIT)
	int3

abort:
	INVOKE_PINOP(OP_ABORT)
	int3

section .bss
cpu_fd: resd 1 
mem_fd: resd 1
message: resb SIZEOF_MESSAGE

	message.type equ message + 4
