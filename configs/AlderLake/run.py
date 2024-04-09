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
import sys
import os

# === PARSEC OPTIONS ===#
sys.argv += [
    "--num-cpus=16",
    "--ruby",
    "--cpu-type=X86O3CPU",
    "--enable-prefetch",
]
# ======================#

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.params import NULL
from m5.util import addToPath, fatal, warn
from gem5.isas import ISA
from gem5.runtime import get_runtime_isa
from CPU import GoldenCove, Gracemont

addToPath("../")

from ruby import Ruby

from common import Options
from common import Simulation
from common import CacheConfig
from common import CpuConfig
from common import ObjectList
from common import MemConfig
from common.FileSystemConfig import config_filesystem
from common.Caches import *

parser = argparse.ArgumentParser()
Options.addCommonOptions(parser)
Options.addSEOptions(parser)

if "--ruby" in sys.argv:
    Ruby.define_options(parser)

args = parser.parse_args()

process = Process(pid=100)
process.executable = args.cmd
process.cwd = os.getcwd()
process.gid = os.getgid()
process.cmd = [args.cmd] + args.options.split()
if args.input:
    process.input = args.input
if args.output:
    process.output = args.output
if args.errout:
    process.errout = args.errout
multiprocesses = [process]

num_pcores = 8
num_ecores = 8

cpus = []
for i in range(num_pcores):
    cpus.append(GoldenCove(cpu_id=len(cpus)))
for i in range(num_ecores):
    cpus.append(Gracemont(cpu_id=len(cpus)))

np = args.num_cpus
mp0_path = multiprocesses[0].executable
system = System(
    cpu=cpus,
    mem_mode="timing",
    mem_ranges=[AddrRange(args.mem_size)],
    cache_line_size=args.cacheline_size,
)

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

pcore_clk_domain = SrcClockDomain(
    clock="3.4GHz", voltage_domain=system.cpu_voltage_domain
)
ecore_clk_domain = SrcClockDomain(
    clock="2.5GHz", voltage_domain=system.cpu_voltage_domain
)

for cpu in cpus:
    if cpu.is_pcore():
        cpu.clk_domain = pcore_clk_domain
    elif cpu.is_ecore():
        cpu.clk_domain = ecore_clk_domain
    else:
        assert False

    cpu.workload = process
    cpu.createThreads()

Ruby.create_system(args, False, system)
assert args.num_cpus == len(system.ruby._cpu_ports)

system.ruby.clk_domain = SrcClockDomain(
    clock=args.ruby_clock, voltage_domain=system.voltage_domain
)
for i in range(np):
    ruby_port = system.ruby._cpu_ports[i]

    # Create the interrupt controller and connect its ports to Ruby
    # Note that the interrupt controller is always present but only
    # in x86 does it have message ports that need to be connected
    system.cpu[i].createInterruptController()

    # Connect the cpu's cache ports to Ruby
    ruby_port.connectCpuPorts(system.cpu[i])

    system.workload = SEWorkload.init_compatible(mp0_path)

CpuConfig.config_scheme(CPUClass, system.cpu, args)
root = Root(full_system=False, system=system)
Simulation.run(args, root, system, None)
