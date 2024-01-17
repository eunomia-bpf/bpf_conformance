import os
import pathlib
import subprocess
import json
from typing import List, TypedDict

EXECUTABLES = [
    "bpftime-llvm",
    # "bpftime-rbpf",
    "bpftime-rbpf-vm",
    # "bpftime-ubpf",
    "bpftime-ubpf-vm",
]
EXECUTABLE_ROOT = pathlib.Path("test_executables")
RUNNER = "./build/bin/bpf_conformance_runner"
DRIVER = "./build/bin/bpftime-test-driver"


class TestcaseResult(TypedDict):
    ok: bool
    name: str
    message: str


def run_and_parse_result(executable: str) -> List[TestcaseResult]:
    ret = subprocess.run(
        [RUNNER, "--test_file_directory", "./tests", "--plugin_path", DRIVER],
        text=True,
        capture_output=True,
        check=False,
        env={
            "RUNTIME_EXECUTABLE": str(EXECUTABLE_ROOT / executable),
            "SPDLOG_LEVEL": "error",
        },
    )
    print(ret.stdout)
    print(ret.stderr)
    result = []
    for line in ret.stdout.strip().splitlines():
        if line.startswith("PASS:"):
            result.append({"ok": True, "name": line.split()[1], "message": line})
        elif line.startswith("FAIL:"):
            result.append({"ok": False, "name": line.split()[1], "message": line})
    return result


def main():
    if not os.path.exists(RUNNER):
        print("You should build the CMake target bpf_conformance_runner")
        exit(1)
    if not os.path.exists(DRIVER):
        print("You should build the CMake target bpftime-test-driver")
        exit(1)

    result = {}
    for exe in EXECUTABLES:
        print("Running", exe)
        curr = run_and_parse_result(exe)
        result[exe] = {
            "pass_count": sum(x["ok"] for x in curr),
            "fail_count": sum(1 - x["ok"] for x in curr),
            "data": curr,
        }
    with open("test-result.json", "w") as f:
        json.dump(result, f)


if __name__ == "__main__":
    main()
