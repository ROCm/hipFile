#!/usr/bin/env python3

import argparse
import concurrent.futures
import os
import subprocess
import sys


def parse_args() -> argparse.Namespace:
    parser: argparse.ArgumentParser = argparse.ArgumentParser()
    parser.add_argument("binary", help="path to test binary")
    parser.add_argument("--ais-capable-dir", default="/tmp", metavar="DIR", help="directory to pass to tests (default: /tmp)")
    parser.add_argument("--test-timeout", type=float, metavar="SECONDS", default=None, help="kill test if it exceeds this many seconds")
    parser.add_argument("-j", type=int, default=1, metavar="N", help="number of tests to run in parallel (default: 1)")
    parser.add_argument("--silent", action="store_true", help="suppress output for passing tests")
    parser.add_argument("--amd-log-level", type=int, choices=range(1, 6), metavar="{1-5}", default=None, help="set AMD_LOG_LEVEL environment variable (1-5)")
    return parser.parse_args()


def list_tests(binary: str) -> list[str]:
    """Return all test names (suite.test) reported by the test binary."""
    result: subprocess.CompletedProcess[str] = subprocess.run(
        [binary, "--gtest_list_tests"],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )

    tests: list[str] = []
    current_suite: str = ""

    line: str
    for line in result.stdout.splitlines():
        if not line:
            continue
        if line[0] != " ":
            # Suite name — ends with '.'
            current_suite = line.split()[0]
        elif len(line) > 2 and line[0] == " " and line[1] == " " and line[2] != " ":
            # Test names are indented exactly two spaces (column 2 = index 2)
            test_name: str = line[2:].split()[0]
            tests.append(current_suite + test_name)

    return tests


def run_test(binary: str, test_name: str, ais_capable_dir: str, timeout: float | None = None, amd_log_level: int | None = None) -> tuple[bool, bool, str]:
    """Run a single test and return (passed, timed_out, output). Kills the process on timeout."""
    env: dict[str, str] = os.environ.copy()
    if amd_log_level is not None:
        env["AMD_LOG_LEVEL"] = str(amd_log_level)
    try:
        result: subprocess.CompletedProcess[str] = subprocess.run(
            [binary, f"--gtest_filter={test_name}", f"--ais-capable-dir={ais_capable_dir}"],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=timeout,
            env=env,
        )
        return result.returncode == 0, False, result.stdout
    except subprocess.TimeoutExpired as e:
        output: str = e.stdout.decode() if isinstance(e.stdout, bytes) else (e.stdout or "")
        return False, True, output + f"\n[TIMEOUT after {timeout}s]"


if __name__ == "__main__":
    args: argparse.Namespace = parse_args()
    tests: list[str] = list_tests(args.binary)
    passed: int = 0
    failed: int = 0
    timed_out: int = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.j) as executor:
        futures: dict[concurrent.futures.Future[tuple[bool, bool, str]], str] = {
            executor.submit(run_test, args.binary, test_name, args.ais_capable_dir, args.test_timeout, args.amd_log_level): test_name
            for test_name in tests
        }
        future: concurrent.futures.Future[tuple[bool, bool, str]]
        for future in concurrent.futures.as_completed(futures):
            test_name = futures[future]
            ok: bool
            did_timeout: bool
            output: str
            ok, did_timeout, output = future.result()
            if ok:
                passed += 1
                if not args.silent:
                    print(f"PASS  {test_name}")
            elif did_timeout:
                timed_out += 1
                print(f"TIMEOUT  {test_name}")
                print(output)
            else:
                failed += 1
                print(f"FAIL  {test_name}")
                print(output)
    print(f"\n{passed} passed, {failed} failed, {timed_out} timed out")
    exit_code: int = 0
    if failed > 0:
        exit_code |= 1
    if timed_out > 0:
        exit_code |= 2
    sys.exit(exit_code)
