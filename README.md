# HACOE - Hardware-Aware Compiler Optimization Engine

HACOE is a research compiler project built on LLVM's New Pass Manager. Its goal is to combine measured host capabilities, static LLVM IR features, explainable optimization decisions, and correctness-first benchmarking.

The current repository is **Phase 0 foundation work**, not a finished autonomous or ML compiler. See the [implementation roadmap](docs/IMPLEMENTATION_ROADMAP.md) for the phased plan and acceptance gates.

## Current capabilities

- Linux/x86 hardware discovery without a hard-coded CPU identity.
- Human-readable and versioned JSON hardware-profile output.
- Static LLVM IR topology analyzer.
- Deterministic, versioned LLVM IR feature documents with CFG, loop, branch,
  call, memory, vector, and opcode data.
- LLVM pass plugins for feature diagnostics and an honest, non-mutating Phase 0 recommendation.
- A validated C/C++ -> LLVM IR -> standard LLVM optimization -> native binary driver.
- Repeatable Linux `perf stat` collection with JSON output and basic dispersion statistics.

## Requirements

- Linux on x86/x86-64 for the current hardware profiler
- CMake 3.20+
- C++20 compiler
- Matching Clang, LLVM development files, `opt`, and CMake LLVM configuration
- Python 3.10+
- Linux `perf` for runtime counter collection

On Arch Linux:

```bash
sudo pacman -S --needed clang llvm cmake ninja python perf
```

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

If CMake cannot locate LLVM, pass the directory containing `LLVMConfig.cmake`:

```bash
cmake -S . -B build -G Ninja -DLLVM_DIR="$(llvm-config --cmakedir)"
```

## Inspect the host

```bash
./build/hacoe-profiler
./build/hacoe-profiler --json > hardware_profile.json
```

## Run the Phase 0 compiler pipeline

Standard LLVM O2 pipeline:

```bash
python3 scripts/hwcc.py tests/vector_add.c --output-dir hacoe-out --run
```

Generate a Phase 1 feature document during compilation:

```bash
python3 scripts/hwcc.py tests/vector_add.c \
  --output-dir hacoe-out \
  --ir-analyzer build/hacoe-ir-analyzer
```

The contract is documented in
[`docs/IR_FEATURE_CONTRACT.md`](docs/IR_FEATURE_CONTRACT.md) and formally
defined by [`schemas/ir-features-v1.schema.json`](schemas/ir-features-v1.schema.json).

Include the diagnostic plugins built by CMake:

```bash
python3 scripts/hwcc.py tests/vector_add.c \
  --output-dir hacoe-out \
  --feature-plugin build/llvm_passes/FeatureExtractor.so \
  --hardware-plugin build/llvm_passes/HardwareAware.so \
  --run
```

Use `--dry-run` to inspect the exact commands without invoking the LLVM toolchain.

## Collect performance counters

```bash
python3 runtime/perf_runner.py hacoe-out/vector_add \
  --repetitions 10 \
  --output perf_metrics.json
```

Some kernels restrict PMU access. HACOE reports that failure instead of suggesting a permanent system-wide security change. Adjust `kernel.perf_event_paranoid` only if you understand the security trade-off.

## Research integrity

HACOE does not count a diagnostic, recommendation, or diagram as an optimization. A future optimization is accepted only when it preserves behavior and is compared reproducibly against Clang/LLVM `-O0`, `-O2`, and `-O3`.
