#include <stdint.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <sys/prctl.h>
#include <asm/prctl.h>

#define STDERR_FILENO 2
#define ENOMEM 12

#include "cpu/pin/message.hh"
#include "syscall.h"
#include "printf.h"
#include "ops.hh"
#include "libc.h"

#ifdef printf
# undef printf
#endif
#define printf(...) do { } while (0)

#define err(fmt, ...)                           \
  do {                                          \
    printf_("error: " fmt " (%d)\n" __VA_OPT__(,) __VA_ARGS__, errno);    \
  } while (0)

// FIXME: Virtual
#define vsyscall_base 0xffffffffff600000ULL
#define vsyscall_end (vsyscall_base + 0x1000)

#define min(a, b) (((a) < (b)) ? (a) : (b))

static void __attribute__((unused))
do_assert_failure(const char *file, int line, const char *desc)
{
    printf_("%s:%d: assertion failed: %s\n", file, line, desc);
}

#ifdef assert
# undef assert
#endif
#define assert(pred) \
    do {             \
    if (!(pred))                                        \
        do_assert_failure(__FILE__, __LINE__, #pred);   \
    } while (false)


static int req_fd;
static int resp_fd;
static int mem_fd;

typedef struct Message Message;

static void
diagnose_ENOMEM(void)
{
    // Read /proc/self/maps.
    int maps_fd;
    if ((maps_fd = open("/proc/self/maps", O_RDONLY)) < 0) {
        err("open: /proc/self/maps");
        return;
    }

    // Count the number of maps present.
    int num_maps = 0;
    while (true) {
        char c;
        ssize_t bytes_read = read(maps_fd, &c, 1);
        if (bytes_read < 0) {
            err("read: /proc/self/maps");
            return;
        }
        if (bytes_read == 0)
            break;
        printf_("%c", c);
        if (c == '\n')
            ++num_maps;
    }

    printf_("ENOMEM: num maps: %d\n", num_maps);
}


void read_all(int fd, void *data_, size_t size) {
    char *data = (char *) data_;
    while (size) {
        const ssize_t bytes_read = read(fd, data, size);
        if (bytes_read < 0) {
            err("read", errno);
            pinop_abort();
        }
        data += bytes_read;
        size -= bytes_read;
    }
}

void write_all(int fd, const void *data_, size_t size) {
    const char *data = (const char *) data_;
    while (size) {
        const ssize_t bytes_written = write(fd, data, size);
        if (bytes_written < 0) {
            err("write");
            pinop_abort();
        }
        data += bytes_written;
        size -= bytes_written;
    }
}

void _putchar(char c) {
    write(STDERR_FILENO, &c, 1);
}

void msg_read(Message *msg) {
    printf("KERNEL: reading request\n");
    read_all(req_fd, msg, sizeof *msg);
    printf("KERNEL: read request\n");
}

void msg_write(const Message *msg) {
    write_all(resp_fd, msg, sizeof *msg);
}

bool
getline(int fd, char *buf, size_t size)
{
    assert(size > 0);
    --size; // Reserve one byte for NUL.
    while (true) {
        char c;
        ssize_t bytes_read = read(fd, &c, 1);
        if (bytes_read < 0) {
            err("read: /proc/self/maps");
            pinop_abort();
        }
        if (bytes_read == 0)
            return false;
        // Only copy the byte if we have space.
        if (size > 0) {
            *buf++ = c;
            --size;
        }
        if (c == '\n') {
            *buf = '\0';
            return true;
        }
    }
}

char *
strchr(char *s, char c)
{
    while (*s) {
        if (*s == c)
            return s;
        ++s;
    }
    return NULL;
}

char *
strsep(char **stringp, char delim)
{
    char *result = *stringp;
    if (result) {
        char *s = strchr(result, delim);
        if (s)
            *s++ = '\0';
        *stringp = s;
    }
    return result;
}

int
hextoint(char c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    printf_("bad hex digit: %c\n", c);
    pinop_abort();
}

uint64_t
parse_hex_u64(const char *s)
{
    uint64_t x = 0;
    while (*s) {
        x *= 16;
        x += hextoint(*s);
        ++s;
    }
    return x;
}

char *
maps_get_line_for_addr(int fd, char *buf, size_t size, uint64_t addr)
{
    while (getline(fd, buf, size)) {
        printf_("%s\n", buf);
        char *s = buf;
        const char *begin_s = strsep(&s, '-');
        const char *end_s = strsep(&s, ' ');
        assert(begin_s && end_s);
        const uint64_t begin = parse_hex_u64(begin_s);
        const uint64_t end = parse_hex_u64(end_s);
        if (begin <= addr && addr < end) {
            // Found a match!
            return s;
        }
    }
    return NULL;
}

void main_event_loop(void) {
    while (true) {
        Message msg;
        msg_read(&msg);

        switch (msg.type) {
          case Ack:
            msg_write(&msg);
            break;

          case SetReg:
            printf("KERNEL: handling SET_REG request\n");
            pinop_set_reg(msg.reg.name, msg.reg.data, msg.reg.size);
            msg.type = Ack;
            msg_write(&msg);
            break;

          case GetReg:
            printf("KERNEL: handling GET_REG request\n");
            pinop_get_reg(msg.reg.name, msg.reg.data, msg.reg.size);
            msg.type = SetReg;
            msg_write(&msg);
            break;

          case Map:
            {
                // Check if vsyscall. This is special case.
                bool is_vsyscall = false;
                if (msg.map.vaddr == vsyscall_base) {
                    printf("KERNEL: fixing up vsyscall mapping 0x%" PRIx64 "->0x%" PRIx64 "\n",
                           msg.map.vaddr, msg.map.paddr);
                    msg.map.vaddr = 0xcafebabe000;
                    is_vsyscall = true;
                }
                
                // printf_("mapping page: %p->%p 0x%lx\n", (void *) msg.map.vaddr, (void *) msg.map.paddr, msg.map.size);
                void *map;
                if ((map = mmap((void *) msg.map.vaddr, msg.map.size, msg.map.prot,
                                MAP_SHARED | MAP_FIXED, mem_fd, msg.map.paddr)) == MAP_FAILED) {
                    err("mmap failed: vaddr=%p size=%zu paddr=%p\n", msg.map.vaddr, msg.map.size, msg.map.paddr);
                    if (errno == ENOMEM)
                        diagnose_ENOMEM();
                    pinop_abort();
                }
                if (map != (void *) msg.map.vaddr) {
                    printf_("error: mmap mapped wrong address\n");
                    pinop_abort();
                }
                printf_("mapped page: %p->%p %p\n", (void *) msg.map.vaddr, (void *) msg.map.paddr, (void *) msg.map.size);
                // printf_("first byte: %02hhx\n", * (uint8_t *) map);
                if (is_vsyscall) {
                    pinop_set_vsyscall_base((void *) vsyscall_base, map);
                }
                msg.type = Ack;
                msg_write(&msg);
            }
            break;

          case Unmap:
            {
                if (munmap((void *) msg.map.vaddr, msg.map.size) < 0) {
                    printf_("error: munmap failed (%d): vaddr=%p size=0x%x\n",
                            errno, msg.map.vaddr, msg.map.size);
                    pinop_abort();
                }
                printf_("unmapped page: %p\n", (void*) msg.map.vaddr);
                msg.type = Ack;
                msg_write(&msg);
            }
            break;

          case Run:
            {
                printf("KERNEL handling RUN request\n");
                struct RunResult result;
                pinop_run(&result);
                Message msg;
                msg.inst_count = pinop_get_instcount();
                switch (result.result) {
                  case RUNRESULT_PAGEFAULT:
                    // Send this up to gem5.
                    printf("KERNEL: got page fault: %" PRIx64 "\n", result.addr);
                    msg.type = PageFault;
                    msg.faultaddr = result.addr;
                    break;

                  case RUNRESULT_SYSCALL:
                    // Send this up to gem5.
                    msg.type = Syscall;
                    break;

                  case RUNRESULT_CPUID:
                    // Send up to gem5.
                    msg.type = Cpuid;
                    break;

                  case RUNRESULT_BREAK:
                    // Send up to gem5.
                    msg.type = Break;
                    break;
                    
                  default:
                    printf_("KERNEL ERROR: unhandled run result: %d\n", result);
                    pinop_abort();
                }

                msg_write(&msg);
            }
            break;

          case SetRegs:
            pinop_set_regs(&msg.regfile);
            msg.type = Ack;
            msg_write(&msg);
            break;

          case GetRegs:
            pinop_get_regs(&msg.regfile);
            msg.type = SetRegs;
            msg_write(&msg);
            break;

          case Exit:
            exit(0);

          case AddSymbol:
            pinop_add_symbol(msg.symbol.name, (void *) msg.symbol.vaddr);
            msg.type = Ack;
            msg_write(&msg);
            break;

          case ExecCommand:
            {
                const size_t bytes = pinop_exec_command(msg.command);
                msg.type = CommandResult;
                msg.command_result_size = bytes;
                msg_write(&msg);

                // TODO: Should just send null-terminated string, once we use buffered files on the gem5 end.
                for (size_t i = 0; i != bytes; ) {
                    char buf[1024];
                    const size_t chunk = min(bytes - i, sizeof buf);
                    pinop_read_command_result(buf, i, chunk);
                    write_all(resp_fd, buf, chunk);
                    i += chunk;
                }
            }
            break;

          case Abort:
            pinop_abort();
            break;
              
          default:
            printf_("error: bad message type (%d)\n", msg.type);
            pinop_abort();
        }

        // printf("KERNEL: handled message, going on to next iteration\n");
    }
}

void main2(void);
void switch_stacks(void *new_stack, void (*f)(void));
void main(void) {
    // Switch stacks.
    printf_("Switching stacks...\n");
    const uint64_t stack_base = 0xcafe0000000;
    const uint64_t stack_size = 0x1000 * 32;
    void *stack;
    if ((stack = mmap((void *) stack_base, stack_size,
                      PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANON, -1, 0)) == MAP_FAILED) {
        printf_("error: failed to map stack (errno=%d)\n", errno);
        pinop_abort();
    }
    switch_stacks((uint8_t *) stack + stack_size, main2); 
}

__attribute__((naked))
void switch_stacks(void *new_stack, void (*f)(void)) {
    asm volatile ("mov %rsp, -8(%rdi)\n" // Save old stack.
                  "mov %rdi, %rsp\n" // Set to new stack.
                  "sub $8, %rsp\n"
                  "call *%rsi\n"
                  "mov (%rsp), %rsp\n"
                  "ret\n"
        );
}

void main2(void) {
    char path[256];
    printf("KERNEL: starting up\n");
    
    // Open request file.
    pinop_get_reqpath(path, sizeof path);
    if ((req_fd = open(path, O_RDONLY)) < 0) {
        printf("error: open failed: %s (%d)\n", path, errno);
        pinop_abort(); 
    }

    printf("KERNEL: opened request file\n");
    
    // Open response file.
    pinop_get_resppath(path, sizeof path);
    if ((resp_fd = open(path, O_WRONLY)) < 0) {
        err("open: %s", path);
        pinop_abort();
    }

    // Open physmem file.
    char mem_path[256];
    pinop_get_mempath(mem_path, sizeof mem_path);
    if ((mem_fd = open(mem_path, O_RDWR)) < 0) {
        err("open: %s", mem_path);
        pinop_abort();
    }

    // Initialize user context.
    pinop_resetuser();

    main_event_loop();

    pinop_exit(0);
}
