#!/usr/bin/env python3
"""End-to-end contract tests for HACOE's IR feature producers."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import tempfile
from pathlib import Path


def run(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, capture_output=True, text=True, check=False)


def require_success(result: subprocess.CompletedProcess[str], label: str) -> None:
    if result.returncode != 0:
        raise AssertionError(
            f"{label} failed with exit code {result.returncode}\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )


def parse_plugin_json(stderr: str) -> dict:
    start = stderr.find("{")
    end = stderr.rfind("}")
    if start < 0 or end < start:
        raise AssertionError(f"plugin did not emit a JSON object:\n{stderr}")
    return json.loads(stderr[start : end + 1])


def assert_contract(document: dict) -> None:
    assert document["schema_version"] == "1.0.0"
    assert re.fullmatch(r"[0-9a-f]{64}", document["module_sha256"])
    assert document["source_file"] == "feature_sample.c"
    assert document["target_triple"] == "x86_64-unknown-linux-gnu"
    assert document["summary"] == {
        "basic_blocks": 4,
        "declarations": 1,
        "defined_functions": 2,
        "instructions": 14,
    }

    functions = {function["name"]: function for function in document["functions"]}
    assert list(function["name"] for function in document["functions"]) == [
        "caller",
        "sum",
    ]

    caller = functions["caller"]
    assert caller["basic_blocks"] == 1
    assert caller["cfg_edges"] == 0
    assert caller["instructions"] == 2
    assert caller["calls"] == 1
    assert caller["indirect_calls"] == 0
    assert caller["opcode_histogram"] == {"call": 1, "ret": 1}

    summation = functions["sum"]
    assert summation["basic_blocks"] == 3
    assert summation["cfg_edges"] == 4
    assert summation["instructions"] == 12
    assert summation["loops"] == 1
    assert summation["maximum_loop_depth"] == 1
    assert summation["conditional_branches"] == 2
    assert summation["unconditional_branches"] == 0
    assert summation["loads"] == 1
    assert summation["stores"] == 0
    assert summation["opcode_histogram"] == {
        "add": 2,
        "br": 2,
        "getelementptr": 1,
        "icmp": 2,
        "load": 1,
        "phi": 3,
        "ret": 1,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--analyzer", required=True)
    parser.add_argument("--opt", required=True)
    parser.add_argument("--plugin", required=True)
    parser.add_argument("--fixture", type=Path, required=True)
    args = parser.parse_args()

    analyzer_command = [args.analyzer, str(args.fixture)]
    first = run(analyzer_command)
    second = run(analyzer_command)
    require_success(first, "first analyzer run")
    require_success(second, "second analyzer run")
    analyzer_document = json.loads(first.stdout)
    assert analyzer_document == json.loads(second.stdout), "analyzer output is not deterministic"
    assert_contract(analyzer_document)

    with tempfile.TemporaryDirectory() as temporary_directory:
        output = Path(temporary_directory) / "features.json"
        file_result = run([*analyzer_command, "--output", str(output)])
        require_success(file_result, "analyzer file-output run")
        assert file_result.stdout == ""
        assert json.loads(output.read_text(encoding="utf-8")) == analyzer_document

        invalid_ir = Path(temporary_directory) / "invalid.ll"
        invalid_ir.write_text("this is not LLVM IR\n", encoding="utf-8")
        invalid_result = run([args.analyzer, str(invalid_ir)])
        assert invalid_result.returncode != 0
        assert "error:" in invalid_result.stderr

    plugin_result = run(
        [
            args.opt,
            "-load-pass-plugin",
            args.plugin,
            "-passes=extract-features",
            "-disable-output",
            str(args.fixture),
        ]
    )
    require_success(plugin_result, "feature-extractor plugin")
    plugin_document = parse_plugin_json(plugin_result.stderr)
    assert plugin_document == analyzer_document, "analyzer and plugin contracts diverged"


if __name__ == "__main__":
    main()
