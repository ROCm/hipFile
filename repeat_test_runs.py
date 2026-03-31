#!/usr/bin/env python3

import argparse
import os
import pathlib
import subprocess
import sys
import uuid


def parse_args() -> argparse.Namespace:
    parser: argparse.ArgumentParser = argparse.ArgumentParser()
    parser.add_argument("binary", help="path to test binary")
    parser.add_argument("--ais-capable-dir", default="/tmp", metavar="DIR", help="directory to pass to tests (default: /tmp)")
    parser.add_argument("--test-timeout", type=float, metavar="SECONDS", default=None, help="kill test if it exceeds this many seconds")
    parser.add_argument("-j", type=int, default=1, metavar="N", help="number of tests to run in parallel (default: 1)")
    parser.add_argument("--silent", action="store_true", help="suppress output for passing tests")
    parser.add_argument("--amd-log-level", type=int, choices=range(1, 6), metavar="{1-5}", default=None, help="set AMD_LOG_LEVEL environment variable (1-5)")
    return parser.parse_args()


if __name__ == "__main__":
    parse_args()  # validate args before starting the loop
    run_id: str = str(uuid.uuid4())
    output_dir: pathlib.Path = pathlib.Path(run_id)
    run_index: int = 0

    while True:
        run_index += 1
        result: subprocess.CompletedProcess[str] = subprocess.run(
            [sys.executable, pathlib.Path(__file__).parent / "run_tests.py"] + sys.argv[1:],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        print(result.stdout, end="")
        if result.returncode != 0:
            output_dir.mkdir(exist_ok=True)
            log_path: pathlib.Path = output_dir / str(run_index)
            log_path.write_text(result.stdout)
            print(f"Output saved to {log_path}")
