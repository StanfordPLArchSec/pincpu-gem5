#pragma once

#ifdef __cplusplus
# include <cstdint>
# include <ostream>
#else
# include <stdint.h>
#endif

#include "regfile.h"

#ifdef __cplusplus

namespace gem5
{

namespace pin
{

#endif

// TODO: Split up into separate requests and respones.
struct Message
{
    enum Type
    {
        Invalid = 0,
        Ack,
        Map,
        SetReg,
        Abort,
        Run,
        PageFault,
        Syscall,
        GetReg,
        Cpuid,
        Exit,
        GetRegs,
        SetRegs,
        Unmap,
        AddSymbol,
        ExecCommand,
        CommandResult,
        Break,
        NumTypes
    } type;
    union
    {
        struct
        {
            // TODO: Use gem5 Addr.
            uint64_t vaddr;
            uint64_t paddr;
            uint64_t size;
            uint64_t prot;
        } map; // For Type::Map

        struct
        {
            char name[63];
            uint8_t size;
            uint8_t data[64];
        } reg; // For Type::SetReg

        uint64_t faultaddr; // for PageFault

        struct PinRegFile regfile; // Valid for GetRegs, SetRegs.

        struct {
            char name[64];
            uint64_t vaddr;
        } symbol;

        char command[64];
        uint64_t command_result_size;
    };

    uint64_t inst_count; // Valid for all responses to RUN requests.
    
#ifdef __cplusplus
    // TODO: Make these members of the pin::CPU class or the PinProcess class.
    void send(int sockfd) const;
    void recv(int sockfd);
#endif
};

#ifdef __cplusplus
std::ostream &operator<<(std::ostream &os, const Message &msg);
#endif

#ifdef __cplusplus
}
}
#endif
