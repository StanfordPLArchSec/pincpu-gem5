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

import argparse
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
from multibin.Util import (
    make_process,
    make_parser,
)

parser = make_parser()
parser.add_argument(
    "--simpoints-json",
    required=True,
    type = os.path.abspath,
    help="Path to SimPoint JSON file under cpt/*",
)
parser.add_argument(
    "--waypoints",
    type = os.path.abspath,
    help = "Path to waypoints list",
)
args = parser.parse_args()
process = make_process(args)

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
cpu = system.cpu[0]

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
cpu.pinToolArgs = "-instcount 1"
if args.waypoints:
    cpu.pinToolArgs += f" -waypoints {args.waypoints} -waypointcount 1"
process.pinInSE = True
cpu.countInsts = True

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

# Parse checkpoints file.
simpoints = None
with open(args.simpoints_json) as f:
    simpoints = json.load(f)
    simpoints = [types.SimpleNamespace(**simpoint) for simpoint in simpoints]

root = Root(full_system=False, system=system)
# Simulation.run(args, root, system, CPUClass)


def get_simpoint_start_inst(simpoint: dict) -> int:
    return simpoint["waypoints"][0]

m5.instantiate()
m5.startup()

m5.options.outdir = os.path.abspath(m5.options.outdir)

# TODO: Support back-to-back intervals.
# We don't support this for now because it makes the logic pretty complex
# when you have to consider interleaved warmups and intervals.
for a, b in zip(simpoints[:-1], simpoints[1:]):
    a = a.interval
    b = b.interval
    assert a < b
    assert a < b - 1

def run_until_waypoint(waypoint):
    break_type = "waypoint" if args.waypoints else "inst"
    cpu.executePinCommand(f"breakpoint {break_type} {waypoint}")
    exit_cause = m5.simulate().getCause()
    if exit_cause != "pin-breakpoint":
        print(f"Unexpected exit cause: {exit_cause}", file=sys.stderr)
        exit(1)
    return int(cpu.executePinCommand(f"instcount"))

for simpoint in simpoints:
    # Assume instruction counting is already set up.
    # Just need to set up instruction count breakpoint.
    wp1, wp2, wp3 = simpoint.waypoints

    # Run until warmup begin.
    inst1 = run_until_waypoint(wp1)

    # Checkpoint.
    short_name = f"cpt.{simpoint.name}"
    path = os.path.join(m5.options.outdir, short_name)
    m5.checkpoint(path)
    m5.stats.dump()
    print(
        "pin-cpt: dumped checkpoint {}".format(simpoint.name),
        file=sys.stderr,
    )

    # Run until warmup end / interval begin.
    inst2 = run_until_waypoint(wp2)

    # Run until interval end.
    inst3 = run_until_waypoint(wp3)

    # Symlink in long gem5 name.
    interval = inst3 - inst2
    warmup = inst2 - inst1
    long_name = f"cpt.simpoint_{int(simpoint.name):02}_inst_{inst1}_weight_{simpoint.weight}_interval_{interval}_warmup_{warmup}"
    os.symlink(short_name, os.path.join(m5.options.outdir, long_name))

# Run to completion.
exit_cause = m5.simulate().getCause()
if exit_cause != "exiting with last active thread context":
    print(f"unexpected exit reason:", exit_cause);
    exit(1)
