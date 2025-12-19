#! /usr/bin/env python3
# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

import argparse
import ctypes
import ctypes.util
import os
import signal
import sys

from bcc import BPF, USDT

# ptrace constants
PTRACE_TRACEME = 0
PTRACE_CONT = 7
PTRACE_SETOPTIONS = 0x4200
PTRACE_O_TRACEEXEC = 0x10

libc = ctypes.CDLL(ctypes.util.find_library("c"), use_errno=True)

def ptrace(request, pid, addr=0, data=0):
    if libc.ptrace(request, pid, addr, data) != 0:
        err = ctypes.get_errno()
        raise OSError(err, os.strerror(err))

def parse_args():
    parser = argparse.ArgumentParser(
        description="hipfile statistics"
    )
    parser.add_argument("-v", "--verbose", action="store_true", help="Enable debug output")
    parser.add_argument("child", nargs=argparse.REMAINDER,
                        help="Child program and arguments (use '--' to separate)")
    return parser.parse_args()

# eBPF program: key is probe name (string), value is hit count
BPF_PROGRAM = r"""
#include <uapi/linux/ptrace.h>

BPF_HASH(ais_io, u64, u64);       // type 0=read, 1=write
BPF_HASH(posix_io, u64, u64);

// ais_io handler
int count_ais_io(struct pt_regs *ctx) {
    u64 type = 0, nbytes = 0;
    bpf_usdt_readarg(1, ctx, &type);
    bpf_usdt_readarg(2, ctx, &nbytes);

    u64 *val, zero = 0;
    val = ais_io.lookup_or_init(&type, &zero);
    (*val) += nbytes;
    return 0;
}

// posix_io handler
int count_posix_io(struct pt_regs *ctx) {
    u64 type = 0, total_bytes = 0;
    bpf_usdt_readarg(1, ctx, &type);
    bpf_usdt_readarg(2, ctx, &total_bytes);

    u64 *val, zero = 0;
    val = posix_io.lookup_or_init(&type, &zero);
    (*val) += total_bytes;
    return 0;
}
"""

def main():
    if os.geteuid() != 0:
        printf("ais-stat must be run as root", file=sys.stderr)
        sys.exit(1)

    args = parse_args()
    verbose = args.verbose
    child_cmd = args.child

    if not child_cmd:
        print("Error: no child program specified", file=sys.stderr)
        sys.exit(1)

    if verbose:
        print(f"Launching child program: '{" ".join(child_cmd)}'")

    pid = os.fork()
    if pid == 0:
        # ---- Child ----
        libc.ptrace(PTRACE_TRACEME, 0, None, None)
        os.kill(os.getpid(), signal.SIGSTOP)  # stop until parent attaches
        os.execvp(child_cmd[0], child_cmd)
    else:
        # ---- Parent ----
        # wait for initial SIGSTOP
        _, status = os.waitpid(pid, 0)
        if not os.WIFSTOPPED(status):
            raise RuntimeError("Child did not stop as expected (initial stop)")

        if verbose:
            print(f"Child {pid} stopped; setting PTRACE options to catch exec")

        # set options to catch exec
        ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACEEXEC)

        # resume child
        ptrace(PTRACE_CONT, pid, 0, 0)

        # wait for exec stop (PTRACE_EVENT_EXEC)
        _, status = os.waitpid(pid, 0)

        # check child state
        if not os.WIFSTOPPED(status):
            raise RuntimeError("Child did not stop after exec as expected")
        if os.WSTOPSIG(status) != signal.SIGTRAP:
            raise RuntimeError(f"Child stopped with unexpected signal after exec: {os.WSTOPSIG(status)}")

        if verbose:
            print(f"Child {pid} exec completed; attaching USDT probes")

        # attach USDT probes
        usdt = USDT(pid=pid)
        probe_list = usdt.enumerate_probes()

        if verbose:
            print(f"Found {len(probe_list)} USDT probes in child:")
            for p in probe_list:
                print(f"  {p.provider}:{p.name}")

        for p in probe_list:
            provider = p.provider.decode() if isinstance(p.provider, bytes) else p.provider
            name = p.name.decode() if isinstance(p.name, bytes) else p.name
            if provider == "hipfile" and name == "ais_io":
                if verbose:
                    print(f"Enabling USDT probe: {provider}:{name} -> count_ais_io")
                usdt.enable_probe(name, "count_ais_io")
            elif provider == "hipfile" and name == "posix_io":
                if verbose:
                    print(f"Enabling USDT probe: {provider}:{name} -> count_posix_io")
                usdt.enable_probe(name, "count_posix_io")

        # load eBPF program
        bpf = BPF(text=BPF_PROGRAM, usdt_contexts=[usdt])

        # resume child
        if verbose:
            print(f"Resuming child {pid} after probe attachment")
        ptrace(PTRACE_CONT, pid, 0, 0)

        # wait for child to finish
        os.waitpid(pid, 0)

        # print results
        print()
        ais_io = bpf["ais_io"]
        posix_io = bpf["posix_io"]
        for io_map, label in [(ais_io, "AIS"), (posix_io, "POSIX")]:
            for t in [0,1]:
                key = ctypes.c_ulong(t)
                val = io_map[key].value if key in io_map else 0
                type_str = "read" if t == 0 else "write"
                print(f"{label} {type_str}: {val} bytes")

if __name__ == "__main__":
    main()
