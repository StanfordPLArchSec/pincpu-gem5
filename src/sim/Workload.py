# Copyright (c) 2023 Arm Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright 2019 Google Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from m5.objects.SimpleMemory import *
from m5.params import *
from m5.SimObject import (
    SimObject,
    cxxMethod,
)


class Workload(SimObject):
    type = "Workload"
    cxx_header = "sim/workload.hh"
    cxx_class = "gem5::Workload"
    abstract = True

    wait_for_remote_gdb = Param.Bool(False, "Wait for a remote GDB connection")
    remote_gdb_port = Param.HostSocket(
        7000, "Default port/socket used for remote GDB connection"
    )

    @cxxMethod
    def sendToGdb(self, message):
        """send a message to the GDB client
        Args:
            message (str): message to send
        """
        pass


class StubWorkload(Workload):
    type = "StubWorkload"
    cxx_header = "sim/workload.hh"
    cxx_class = "gem5::StubWorkload"

    entry = Param.Addr(0, "Dummy entry point for this workload.")
    byte_order = Param.ByteOrder(
        "little", "Dummy byte order for this workload."
    )


class KernelPanicOopsBehaviour(ScopedEnum):
    "Define what gem5 should do after a Kernel Panic or Oops."

    vals = [
        "Continue",
        "DumpDmesgAndContinue",
        "DumpDmesgAndExit",
        "DumpDmesgAndPanic",
    ]


class KernelWorkload(Workload):
    type = "KernelWorkload"
    cxx_header = "sim/kernel_workload.hh"
    cxx_class = "gem5::KernelWorkload"

    object_file = Param.String("", "File that contains the kernel code")
    extras = VectorParam.String([], "Additional object files to load")
    extras_addrs = VectorParam.Addr(
        [], "Load addresses for additional object files"
    )

    addr_check = Param.Bool(
        True,
        "whether to bounds check kernel addresses (disable for baremetal)",
    )
    load_addr_mask = Param.UInt64(
        0xFFFFFFFFFFFFFFFF,
        "Mask to apply to kernel addresses. If zero, "
        "auto-calculated to be the most restrictive.",
    )
    load_addr_offset = Param.UInt64(0, "Address to offset the kernel with")

    command_line = Param.String("a", "boot flags to pass to the kernel")

    on_panic = Param.KernelPanicOopsBehaviour(
        "DumpDmesgAndExit",
        "Define how gem5 should behave after a Linux Kernel Panic. "
        "Handler might not be implemented for all architectures.",
    )

    on_oops = Param.KernelPanicOopsBehaviour(
        "DumpDmesgAndExit",
        "Define how gem5 should behave after a Linux Kernel Oops. "
        "Handler might not be implemented for all architectures.",
    )


class SEWorkloadMeta(type(Workload)):
    all_se_workload_classes = []

    def __new__(mcls, name, bases, dct):
        cls = super().__new__(mcls, name, bases, dct)
        SEWorkloadMeta.all_se_workload_classes.append(cls)
        return cls


class SEWorkload(Workload, metaclass=SEWorkloadMeta):
    type = "SEWorkload"
    cxx_header = "sim/se_workload.hh"
    cxx_class = "gem5::SEWorkload"
    abstract = True

    @classmethod
    def _is_compatible_with(cls, obj):
        return False

    @classmethod
    def find_compatible(cls, path):
        """List the SE workloads compatible with the binary at path"""

        from _m5 import object_file

        obj = object_file.create(path)

        if obj is None:
            raise ValueError(
                f"{path}: the file is not an object file compatible with gem5."
            )

        options = list(
            filter(
                lambda wld: wld._is_compatible_with(obj),
                SEWorkloadMeta.all_se_workload_classes,
            )
        )

        return options

    @classmethod
    def init_compatible(cls, path, *args, **kwargs):
        """Construct the only SE workload compatible with the binary at path"""

        options = SEWorkload.find_compatible(path)

        if len(options) > 1:
            raise ValueError("More than one SE workload is compatible with %s")
        elif len(options) < 1:
            raise ValueError("No SE workload is compatible with %s", path)

        return options[0](*args, **kwargs)
