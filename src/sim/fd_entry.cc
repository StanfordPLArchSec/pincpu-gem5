/*
 * Copyright (c) 2016 Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "sim/fd_entry.hh"

#include <map>
#include <unistd.h>

#include "sim/serialize.hh"

namespace gem5
{

void
FDEntry::serialize(CheckpointOut &cp) const
{
    SERIALIZE_SCALAR(_closeOnExec);
}

void
FDEntry::unserialize(CheckpointIn &cp)
{
    UNSERIALIZE_SCALAR(_closeOnExec);
}

void
FileFDEntry::serialize(CheckpointOut &cp) const
{
    SERIALIZE_SCALAR(_closeOnExec);
    SERIALIZE_SCALAR(_flags);
    SERIALIZE_SCALAR(_fileName);
    SERIALIZE_SCALAR(_fileOffset);
    SERIALIZE_SCALAR(_mode);
}

void
FileFDEntry::unserialize(CheckpointIn &cp)
{
    UNSERIALIZE_SCALAR(_closeOnExec);
    UNSERIALIZE_SCALAR(_flags);
    UNSERIALIZE_SCALAR(_fileName);
    UNSERIALIZE_SCALAR(_fileOffset);
    UNSERIALIZE_SCALAR(_mode);
}

void
PipeFDEntry::serialize(CheckpointOut &cp) const
{
    SERIALIZE_SCALAR(_closeOnExec);
    SERIALIZE_SCALAR(_flags);
    //SERIALIZE_SCALAR(_pipeEndType);
}

void
PipeFDEntry::unserialize(CheckpointIn &cp)
{
    UNSERIALIZE_SCALAR(_closeOnExec);
    UNSERIALIZE_SCALAR(_flags);
    //UNSERIALIZE_SCALAR(_pipeEndType);
}

void
DeviceFDEntry::serialize(CheckpointOut &cp) const
{
    SERIALIZE_SCALAR(_closeOnExec);
    //SERIALIZE_SCALAR(_driver);
    SERIALIZE_SCALAR(_fileName);
}

void
DeviceFDEntry::unserialize(CheckpointIn &cp)
{
    UNSERIALIZE_SCALAR(_closeOnExec);
    //UNSERIALIZE_SCALAR(_driver);
    UNSERIALIZE_SCALAR(_fileName);
}

void
FDEntry::print(std::ostream &os) const
{
    static const std::map<FDClass, std::string> class_map = {
        {fd_base, "base"},
        {fd_hb, "hb"},
        {fd_file, "file"},
        {fd_pipe, "pipe"},
        {fd_device, "device"},
        {fd_socket, "socket"},
        {fd_null, "null"},
    };
    os << "class=" << class_map.at(_class) << " coe=" << _closeOnExec;
}

void
HBFDEntry::print(std::ostream &os) const
{
    FDEntry::print(os);
    os << " sim_fd=" << std::dec << _simFD << " flags=0x" << std::hex << _flags;

    char buf[64];
    sprintf(buf, "/proc/self/fd/%d", getSimFD());
    char link[1024];
    const ssize_t len = readlink(buf, link, sizeof link);
    if (len < 0)
        panic("readlink: %s\n", buf);
    link[len] = '\0';
    os << " procfs=" << link;
}

void
FileFDEntry::print(std::ostream &os) const
{
    HBFDEntry::print(os);
    os << " filename=" << _fileName << " offset=" << std::dec << _fileOffset << " mode=0x" << std::hex << _mode;
}

void
PipeFDEntry::print(std::ostream &os) const
{
    HBFDEntry::print(os);
    os << " pipe_read_source=" << std::dec << _pipeReadSource << " pipe_end_type=";
    switch (_pipeEndType) {
      case read:
        os << "read";
        break;
      case write:
        os << "write";
        break;
      default: panic("Bad pipe end type!\n");
    }
}

void
DeviceFDEntry::print(std::ostream &os) const
{
    FDEntry::print(os);
    os << " filename=" << _fileName;
}

void
SocketFDEntry::print(std::ostream &os) const
{
    HBFDEntry::print(os);
    os << " domain=" << std::dec << _domain << " type=" << _type << " protocol=" << _protocol;
}

} // namespace gem5
