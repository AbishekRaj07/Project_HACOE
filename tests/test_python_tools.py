import importlib.util
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


hwcc = load_module("hwcc", ROOT / "scripts" / "hwcc.py")
perf_runner = load_module("perf_runner", ROOT / "runtime" / "perf_runner.py")


class HwccTests(unittest.TestCase):
    def test_dry_run_builds_frontend_opt_and_link_commands(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            directory = Path(temporary_directory)
            source = directory / "sample.c"
            source.write_text("int main(void) { return 0; }\n", encoding="utf-8")
            args = hwcc.parse_args(
                [str(source), "--output-dir", str(directory / "out"), "--dry-run"]
            )

            commands = list(hwcc.build_commands(args))

            self.assertEqual(commands[0][0], "clang")
            self.assertIn("-emit-llvm", commands[0])
            self.assertEqual(commands[1][0], "opt")
            self.assertIn("-passes=default<O2>", commands[1])
            self.assertEqual(commands[2][0], "clang")

    def test_inserts_explicit_ir_analyzer_after_frontend(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            directory = Path(temporary_directory)
            source = directory / "sample.c"
            source.write_text("int main(void) { return 0; }\n", encoding="utf-8")
            analyzer = directory / "hacoe-ir-analyzer"
            analyzer.write_text("test placeholder\n", encoding="utf-8")
            args = hwcc.parse_args(
                [
                    str(source),
                    "--output-dir",
                    str(directory / "out"),
                    "--ir-analyzer",
                    str(analyzer),
                    "--dry-run",
                ]
            )

            commands = list(hwcc.build_commands(args))

            self.assertEqual(commands[1][0], str(analyzer.resolve()))
            self.assertEqual(commands[1][-2], "--output")
            self.assertTrue(commands[1][-1].endswith("sample.features.json"))

    def test_rejects_unsupported_source_extension(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            source = Path(temporary_directory) / "sample.rs"
            source.write_text("fn main() {}\n", encoding="utf-8")
            args = hwcc.parse_args([str(source), "--dry-run"])
            with self.assertRaises(hwcc.PipelineError):
                list(hwcc.build_commands(args))


class PerfParserTests(unittest.TestCase):
    def test_parses_available_and_unsupported_counters(self):
        metrics = perf_runner.parse_perf_csv(
            "1000,,cycles,1,100.00,,\n<not supported>,,cache-misses,0,0.00,,\n"
        )
        self.assertEqual(metrics["cycles"], 1000.0)
        self.assertIsNone(metrics["cache-misses"])


if __name__ == "__main__":
    unittest.main()
