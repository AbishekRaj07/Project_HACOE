#!/usr/bin/env python3
"""HACOE Phase 0 compiler driver.

The driver is deliberately conservative: it builds LLVM IR, runs optional
diagnostic plugins, applies a standard LLVM pipeline, and links a binary. It
does not claim a hardware-specific speedup until later phases provide measured
decisions and benchmark validation.
"""

from __future__ import annotations

import argparse
import shlex
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Iterable, Sequence


class PipelineError(RuntimeError):
    """Raised for a precise, user-actionable pipeline failure."""


def command_text(command: Sequence[str]) -> str:
    return shlex.join(str(part) for part in command)


def require_tool(name: str) -> str:
    resolved = shutil.which(name)
    if resolved is None:
        raise PipelineError(
            f"required tool '{name}' was not found in PATH; install a matching "
            "Clang/LLVM toolchain and retry"
        )
    return resolved


def run(command: Sequence[str], *, dry_run: bool = False) -> None:
    print(f"[HACOE] {command_text(command)}")
    if dry_run:
        return
    try:
        subprocess.run(command, check=True)
    except subprocess.CalledProcessError as exc:
        raise PipelineError(
            f"command failed with exit code {exc.returncode}: {command_text(command)}"
        ) from exc


def validate_source(source: Path) -> str:
    if not source.is_file():
        raise PipelineError(f"source file does not exist: {source}")
    if source.suffix in {".c"}:
        return "clang"
    if source.suffix in {".cc", ".cpp", ".cxx"}:
        return "clang++"
    raise PipelineError(
        f"unsupported source extension '{source.suffix}'; Phase 0 supports C and C++"
    )


def optional_plugin_command(
    opt: str, ir_path: Path, plugin: Path | None, pass_name: str
) -> list[str] | None:
    if plugin is None:
        return None
    if not plugin.is_file():
        raise PipelineError(f"pass plugin does not exist: {plugin}")
    return [
        opt,
        "-load-pass-plugin",
        str(plugin),
        f"-passes={pass_name}",
        "-disable-output",
        str(ir_path),
    ]


def build_commands(args: argparse.Namespace) -> Iterable[list[str]]:
    source = args.source.resolve()
    compiler_name = validate_source(source)
    compiler = compiler_name if args.dry_run else require_tool(compiler_name)
    opt = "opt" if args.dry_run else require_tool("opt")

    output_dir = args.output_dir.resolve()
    raw_ir = output_dir / f"{source.stem}.raw.ll"
    optimized_ir = output_dir / f"{source.stem}.{args.optimization}.ll"
    binary = output_dir / source.stem

    yield [
        compiler,
        "-O0",
        "-Xclang",
        "-disable-O0-optnone",
        "-S",
        "-emit-llvm",
        str(source),
        "-o",
        str(raw_ir),
    ]

    feature_command = optional_plugin_command(
        opt, raw_ir, args.feature_plugin, "extract-features"
    )
    if feature_command:
        yield feature_command

    hardware_command = optional_plugin_command(
        opt, raw_ir, args.hardware_plugin, "hw-aware-opt"
    )
    if hardware_command:
        yield hardware_command

    yield [
        opt,
        "-S",
        f"-passes=default<{args.optimization}>",
        str(raw_ir),
        "-o",
        str(optimized_ir),
    ]
    yield [compiler, str(optimized_ir), "-o", str(binary)]

    if args.run_binary:
        yield [str(binary), *args.program_arg]


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Compile one C/C++ source through the HACOE Phase 0 pipeline."
    )
    parser.add_argument("source", type=Path)
    parser.add_argument(
        "--output-dir", type=Path, default=Path("hacoe-out"), help="artifact directory"
    )
    parser.add_argument(
        "--optimization",
        choices=("O0", "O1", "O2", "O3", "Os", "Oz"),
        default="O2",
        help="standard LLVM pipeline used after HACOE diagnostics",
    )
    parser.add_argument("--feature-plugin", type=Path)
    parser.add_argument("--hardware-plugin", type=Path)
    parser.add_argument("--run", dest="run_binary", action="store_true")
    parser.add_argument(
        "--program-arg", action="append", default=[], help="argument passed to the output binary"
    )
    parser.add_argument("--dry-run", action="store_true")
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        args.output_dir.mkdir(parents=True, exist_ok=True)
        for command in build_commands(args):
            run(command, dry_run=args.dry_run)
    except PipelineError as error:
        print(f"hwcc: error: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
