#!/usr/bin/env python3

import argparse
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT_DIR / "build"
COMPILER = BUILD_DIR / "compilador"
TEST_DIR = ROOT_DIR / "tests"
ARTIFACT_DIR = ROOT_DIR / "test-artifacts"


@dataclass(frozen=True)
class CommandResult:
    returncode: int
    stdout: str
    stderr: str


def run(command, *, cwd=ROOT_DIR, input_text=None) -> CommandResult:
    result = subprocess.run(
        command,
        cwd=cwd,
        input=input_text,
        capture_output=True,
        text=True,
    )
    return CommandResult(result.returncode, result.stdout, result.stderr)


def read_optional(path: Path, default: str = "") -> str:
    return path.read_text() if path.exists() else default


def read_expected_exit_code(test: Path) -> int:
    path = test / "exit_code.txt"
    if not path.exists():
        return 0
    return int(path.read_text().strip())


def compare_text(label: str, expected: str, actual: str) -> list[str]:
    if expected == actual:
        return []
    return [
        f"{label} mismatch",
        "expected:",
        expected,
        "actual:",
        actual,
    ]


def check_contains(label: str, needle_path: Path, haystack: str) -> list[str]:
    if not needle_path.exists():
        return []
    needle = needle_path.read_text().rstrip("\n")
    if needle in haystack:
        return []
    return [
        f"{label} missing required text",
        "required:",
        needle,
        "actual:",
        haystack,
    ]


def check_not_contains(label: str, needle_path: Path, haystack: str) -> list[str]:
    if not needle_path.exists():
        return []
    needle = needle_path.read_text().rstrip("\n")
    if needle not in haystack:
        return []
    return [
        f"{label} contained forbidden text",
        "forbidden:",
        needle,
        "actual:",
        haystack,
    ]


def check_assembly_expectations(test: Path, assembly_path: Path) -> list[str]:
    assembly = assembly_path.read_text()
    failures = []
    failures.extend(
        check_contains("assembly", test / "assembly_contains.txt", assembly)
    )
    failures.extend(
        check_not_contains(
            "assembly", test / "assembly_not_contains.txt", assembly
        )
    )
    return failures


def discover(category: str) -> list[Path]:
    directory = TEST_DIR / category
    if not directory.exists():
        return []
    return sorted(test for test in directory.iterdir() if (test / "source.c").exists())


def compile_source(test: Path, assembly_path: Path) -> CommandResult:
    return run([str(COMPILER), str(test / "source.c"), "-o", str(assembly_path)])


def assemble(test: Path, assembly_path: Path, executable_path: Path) -> CommandResult:
    command = ["gcc", "-no-pie", str(assembly_path)]
    support = test / "support.c"
    if support.exists():
        command.append(str(support))
    command.extend(["-o", str(executable_path)])
    return run(command)


def run_executable(test: Path, executable_path: Path) -> CommandResult:
    stdin = read_optional(test / "stdin.txt")
    return run([str(executable_path)], input_text=stdin)


def validate_run_test(test: Path, work_dir: Path) -> list[str]:
    assembly = work_dir / "program.s"
    executable = work_dir / "program"

    compiled = compile_source(test, assembly)
    if compiled.returncode != 0:
        return ["compiler failed", compiled.stdout, compiled.stderr]
    failures = check_assembly_expectations(test, assembly)
    if failures:
        return failures

    assembled = assemble(test, assembly, executable)
    if assembled.returncode != 0:
        return ["assembler failed", assembled.stdout, assembled.stderr]

    executed = run_executable(test, executable)
    expected_exit_code = read_expected_exit_code(test)
    failures = []
    if executed.returncode != expected_exit_code:
        failures.append(
            f"exit code mismatch: expected {expected_exit_code}, got {executed.returncode}"
        )
    failures.extend(
        compare_text("stdout", read_optional(test / "stdout.txt"), executed.stdout)
    )
    failures.extend(
        compare_text("stderr", read_optional(test / "stderr.txt"), executed.stderr)
    )
    return failures


def validate_compile_only_test(test: Path, work_dir: Path) -> list[str]:
    assembly = work_dir / "program.s"
    compiled = compile_source(test, assembly)
    if compiled.returncode != 0:
        return ["compiler failed", compiled.stdout, compiled.stderr]
    return check_assembly_expectations(test, assembly)


def validate_compile_fail_test(test: Path, work_dir: Path) -> list[str]:
    compiled = compile_source(test, work_dir / "program.s")
    if compiled.returncode == 0:
        return ["compiler succeeded unexpectedly", compiled.stdout, compiled.stderr]

    failures = []
    failures.extend(check_contains("stdout", test / "stdout_contains.txt", compiled.stdout))
    failures.extend(check_contains("stderr", test / "stderr_contains.txt", compiled.stderr))
    return failures


def build_compiler() -> int:
    result = run(["make", "-C", str(ROOT_DIR)])
    if result.returncode == 0:
        return 0
    print(result.stdout, end="")
    print(result.stderr, end="", file=sys.stderr)
    return result.returncode


def run_suite(*, keep_artifacts: bool) -> int:
    validators = {
        "run": validate_run_test,
        "compile_only": validate_compile_only_test,
        "compile_fail": validate_compile_fail_test,
    }

    if keep_artifacts:
        if ARTIFACT_DIR.exists():
            shutil.rmtree(ARTIFACT_DIR)
        ARTIFACT_DIR.mkdir()

    total = 0
    failed = 0

    with tempfile.TemporaryDirectory(prefix="compilador-tests-") as temp_root:
        temp_root_path = Path(temp_root)
        for category, validator in validators.items():
            for test in discover(category):
                total += 1
                work_dir = (
                    ARTIFACT_DIR / category / test.name
                    if keep_artifacts
                    else temp_root_path / category / test.name
                )
                work_dir.mkdir(parents=True, exist_ok=True)
                failures = validator(test, work_dir)
                if failures:
                    failed += 1
                    print(f"FAIL {category}/{test.name}", file=sys.stderr)
                    for failure in failures:
                        if failure:
                            print(failure, file=sys.stderr)
                    continue
                print(f"PASS {category}/{test.name}")

    if failed:
        print(f"{failed}/{total} test(s) failed.", file=sys.stderr)
        return 1

    print(f"{total} test(s) passed.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Run compiler regression tests.")
    parser.add_argument(
        "--no-build",
        action="store_true",
        help="Do not build the compiler before running tests.",
    )
    parser.add_argument(
        "--keep-artifacts",
        action="store_true",
        help="Keep generated assembly and executables under test-artifacts/.",
    )
    args = parser.parse_args()

    if not args.no_build and build_compiler() != 0:
        return 1
    return run_suite(keep_artifacts=args.keep_artifacts)


if __name__ == "__main__":
    sys.exit(main())
