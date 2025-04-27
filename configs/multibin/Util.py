import os
from argparse import ArgumentParser
from m5.objects import Process
from m5.util import addToPath

addToPath("..")

from common import (
    Options
)

def make_parser() -> ArgumentParser:
    parser = ArgumentParser()
    parser.add_argument("--chdir", default=".", help="Set working directory of simulated process")
    parser.add_argument("--max-stack-size", type=str, default="64MB")
    Options.addCommonOptions(parser)
    Options.addSEOptions(parser)
    parser.add_argument("cmd", help="Executable to simulate")
    parser.add_argument("args", nargs="*", help="Arguments to pass to executable")

    # FIXME: Shouldn't hard-code this. Should generate the path at build time, like other
    # m5 paths.
    gem5_root = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
    parser.add_argument(
        "--pin",
        default=os.path.join(gem5_root, "pin", "pin"),
        help="Path to Intel Pin executable",
    )
    parser.add_argument(
        "--pin-tool",
        default=os.path.join(gem5_root, "pintool", "build", "libclient.so"),
        help="Path to host PinTool",
    ),
    parser.add_argument(
        "--pin-kernel",
        default=os.path.join(gem5_root, "pintool", "build", "kernel"),
        help="Path to Pin guest kernel",
    )

    parser.add_argument("--stdin", default="/dev/null")
    parser.add_argument("--stdout", default="stdout.txt")
    parser.add_argument("--stderr", default="stderr.txt")

    return parser

def make_process(args) -> Process:
    process = Process(pid=100)
    process.executable = args.cmd
    process.cwd = os.path.abspath(args.chdir)
    process.gid = os.getgid()
    process.maxStackSize = args.max_stack_size

    # Clear out the environment, unless it's set.
    if args.env == "host":
        process.env = [f"{key}={value}" for key, value in os.environ.items()]
    elif args.env:
        with open(args.env) as f:
            process.env = [line.rstrip() for line in f]
    else:
        process.env = []

    process.cmd = [args.cmd, *args.args]

    # Get stdin path.
    process.input = args.stdin if os.path.isabs(args.stdin) \
        else os.path.join(args.chdir, args.stdin)
    process.output = args.stdout
    process.errout = args.stderr

    # Set uid, gid, euid, egid.
    process.uid = os.getuid()
    process.euid = os.geteuid()
    process.gid = os.getgid()
    process.egid = os.getegid()

    return process
