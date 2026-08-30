#!/usr/bin/env python3
"""Collect repeatable Linux perf measurements for a compiled benchmark."""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path
from statistics import mean, pstdev
from typing import Sequence


EVENTS = "cycles,instructions,branches,branch-misses,cache-references,cache-misses"


def parse_perf_csv(stderr: str) -> dict[str, float | None]:
    metrics: dict[str, float | None] = {}
    for line in stderr.splitlines():
        fields = [field.strip() for field in line.split(",")]
        if len(fields) < 3:
            continue
        value, _, event = fields[:3]
        if value in {"<not counted>", "<not supported>"}:
            metrics[event] = None
            continue
        try:
            metrics[event] = float(value)
        except ValueError:
            continue
    return metrics


def collect(binary: Path, repetitions: int, program_args: Sequence[str]) -> dict:
    if shutil.which("perf") is None:
        raise RuntimeError("'perf' was not found in PATH")
    if not binary.is_file():
        raise RuntimeError(f"binary does not exist: {binary}")

    runs = []
    command = [
        "perf",
        "stat",
        "-x,",
        "-e",
        EVENTS,
        "--",
        str(binary.resolve()),
        *program_args,
    ]
    for _ in range(repetitions):
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        if result.returncode != 0:
            raise RuntimeError(
                "perf failed; check kernel.perf_event_paranoid and event availability: "
                + result.stderr.strip()
            )
        runs.append(parse_perf_csv(result.stderr))

    summary = {}
    for event in EVENTS.split(","):
        values = [run[event] for run in runs if run.get(event) is not None]
        summary[event] = {
            "available": bool(values),
            "mean": mean(values) if values else None,
            "population_stddev": pstdev(values) if len(values) > 1 else 0.0 if values else None,
        }

    return {
        "schema_version": 1,
        "binary": str(binary.resolve()),
        "repetitions": repetitions,
        "runs": runs,
        "summary": summary,
    }


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", type=Path)
    parser.add_argument("--repetitions", type=int, default=5)
    parser.add_argument("--program-arg", action="append", default=[])
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)

    if args.repetitions < 1:
        parser.error("--repetitions must be at least 1")
    try:
        report = collect(args.binary, args.repetitions, args.program_arg)
    except RuntimeError as error:
        print(f"perf_runner: error: {error}", file=sys.stderr)
        return 2

    payload = json.dumps(report, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(payload, encoding="utf-8")
    else:
        print(payload, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
