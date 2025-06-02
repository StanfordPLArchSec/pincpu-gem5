# Copyright (c) 2012-2013 ARM Limited
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
# Copyright (c) 2006-2008 The Regents of The University of Michigan
# All rights reserved.
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

# Simple test script
#
# "m5 test.py"

# TODO: Compress bbv.txt. It's going to get huge for longer-running benchmarks.

import argparse
import collections
import json
import os
import sys
import types

from common import (
    CacheConfig,
    CpuConfig,
    MemConfig,
    ObjectList,
    Options,
    Simulation,
)
from common.Caches import *
from common.FileSystemConfig import config_filesystem
from multibin.Util import (
    make_parser,
    make_process,
)

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.params import NULL
from m5.util import (
    addToPath,
    fatal,
    warn,
)

from gem5.isas import ISA

parser = make_parser()
parser.add_argument("--interval", type=int, required=True)
parser.add_argument("--warmup", type=int, required=True)
args = parser.parse_args()
assert args.interval > args.warmup
process = make_process(args)

print(os.listdir(m5.options.outdir), file=sys.stderr)

# NHM-FIXME: Just read the kvm cpu directly?
# To get mem mode: CPUClass.memory_mode()
CPUClass = ObjectList.cpu_list.get("X86PinCPU")
assert int(CPUClass.numThreads) == 1
assert not args.smt
assert args.num_cpus == 1

# NHM-FIXME
np = 1
mp0_path = process.executable
system = System(
    cpu=[CPUClass(cpu_id=i) for i in range(np)],
    mem_mode=CPUClass.memory_mode(),
    mem_ranges=[AddrRange(args.mem_size)],
    cache_line_size=args.cacheline_size,
)
system.shared_backstore = f"physmem"
system.auto_unlink_shared_backstore = True
system.use_pagelist = True
cpu = system.cpu[0]
cpu.pinToolArgs = f"-hfi {int(args.hfi)}"

# Create a top-level voltage domain
system.voltage_domain = VoltageDomain(voltage=args.sys_voltage)

# Create a source clock for the system and set the clock period
system.clk_domain = SrcClockDomain(
    clock=args.sys_clock, voltage_domain=system.voltage_domain
)

# Create a CPU voltage domain
system.cpu_voltage_domain = VoltageDomain()

# Create a separate clock domain for the CPUs
system.cpu_clk_domain = SrcClockDomain(
    clock=args.cpu_clock, voltage_domain=system.cpu_voltage_domain
)

# If elastic tracing is enabled, then configure the cpu and attach the elastic
# trace probe
if args.elastic_trace_en:
    CpuConfig.config_etrace(CPUClass, system.cpu, args)


# Set pin params.
cpu = system.cpu[0]
cpu.countInsts = True

# for cpu in system.cpu:
#     cpu.usePerf = True
process.pinInSE = True

# All cpus belong to a common cpu_clk_domain, therefore running at a common
# frequency.
cpu.clk_domain = system.cpu_clk_domain

system.m5ops_base = max(0xFFFF0000, Addr(args.mem_size).getValue())

process.maxStackSize = args.max_stack_size

# NHM-FIXME
cpu.workload = process
cpu.createThreads()

# NHM-FIXME
MemClass = Simulation.setMemClass(args)
system.membus = SystemXBar()
system.system_port = system.membus.cpu_side_ports
CacheConfig.config_cache(args, system)
MemConfig.config_mem(args, system)
config_filesystem(system, args)

system.workload = SEWorkload.init_compatible(mp0_path)

root = Root(full_system=False, system=system)
m5.instantiate()
m5.startup()
# exit_sysnos = [
#     60,  # exit
#     231,  # exit_group
# ]
# for exit_sysno in exit_sysnos:
#     cpu.executePinCommand(f"sysbreak {exit_sysno}")

clean_exit_cause = "exiting with last active thread context"
break_exit_cause = "pin-breakpoint"


class Exit(BaseException):
    pass

def run_for_n(counter: str, n: int):
    # TODO: Should standardize command to 'count <name>'
    count = int(cpu.executePinCommand(f"{counter}count"))
    cpu.executePinCommand(f"breakpoint {counter} {count + n}")
    exit_cause = m5.simulate().getCause()
    if exit_cause == clean_exit_cause:
        raise Exit()
    if exit_cause != break_exit_cause:
        print(
            f"pin-bbv: expected exit cause '{break_exit_cause}', got '{exit_cause}'",
            file=sys.stderr,
        )
        exit(1)

# TODO: Remove copy.
interval = args.interval
warmup = args.warmup
cpt_count = 0

print(os.listdir(m5.options.outdir), file=sys.stderr)


try:
    # Run for one instruction just to avoid weirdness from checkpointing before
    # executing anything. Not sure if this would be a problem or not, but doing
    # this anyway, since it doesn't hurt.
    run_for_n("inst", 10000)

    print(os.listdir(m5.options.outdir), file=sys.stderr)

    def take_checkpoint(warmup, interval):
        global cpt_count
        short_name = f"cpt.{cpt_count}"
        path = os.path.join(m5.options.outdir, short_name)
        m5.checkpoint(path)
        print(f"pin-chunk: dumped checkpoint {cpt_count}",
              file=sys.stderr)

        # Symlink to long SimPoint-style name.
        inst = int(cpu.executePinCommand("instcount"))
        long_name = f"cpt.simpoint_{cpt_count}_inst_{inst}_weight_0_interval_{interval}_warmup_{warmup}"
        os.symlink(short_name, os.path.join(m5.options.outdir, long_name))
              
        cpt_count += 1

    # Take the first checkpoint at start.
    take_checkpoint(warmup=1, interval=interval)

    # Run for interval-warmup
    run_for_n("inst", interval - warmup)

    while True:
        take_checkpoint(warmup=warmup, interval=interval)
        run_for_n("inst", interval)
        
except Exit:
    pass

print(f"Exiting normally")
