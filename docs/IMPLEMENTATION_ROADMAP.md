# HACOE Implementation Roadmap

This roadmap converts the architecture diagrams into testable engineering phases. A phase is complete only when its acceptance gate passes; diagram-only or hard-coded behavior does not count.

## Scope rules

- Establish a deterministic rule-based baseline before adding machine learning.
- Never claim an optimization unless correctness is preserved and benchmark evidence exists.
- Keep hardware discovery, IR analysis, optimization decisions, pass execution, and evaluation as separate modules with versioned data contracts.
- Compare against Clang/LLVM `-O0`, `-O2`, and `-O3`; HACOE is not successful merely because it beats `-O0`.
- Record compiler version, target triple, CPU model, benchmark input, repetitions, and raw measurements for every result.

## Phase 0 - Reproducible foundation

**Architecture coverage:** deployment baseline, compiler lifecycle skeleton, validation prerequisites.

Deliverables:

- Root CMake build for the profiler, IR analyzer, and LLVM pass plugins.
- Toolchain/version checks with actionable failure messages.
- A CLI driver with explicit inputs, output directory, target CPU, and optimization mode.
- Unit tests for orchestration and smoke tests for native compilation.
- Continuous integration for formatting, Python tests, build, and a minimal end-to-end run.
- Removal of hard-coded CPU identity and simulated optimization inputs.

Acceptance gate:

1. A clean Linux environment can configure and build from documented commands.
2. `hacoe <source.c>` either produces a runnable binary or fails with a precise diagnostic.
3. CI passes on the default branch.
4. No hardware value or optimization decision is fabricated.

## Phase 1 - Frontend and LLVM IR intelligence

**Architecture coverage:** Diagram 3A, Level-1 frontend/IR layer, static program analyzer.

Deliverables:

- C/C++ source-to-LLVM-IR pipeline; Rust support is deferred until the C/C++ path is stable.
- Module, function, basic-block, loop, branch, call, memory-access, and instruction-mix features.
- Stable JSON schema with schema version, module hash, target triple, and per-function records.
- Tests using small kernels with known feature counts.

Acceptance gate:

1. Feature extraction is deterministic for identical IR.
2. Golden tests verify counts and schema.
3. Invalid or incompatible IR is rejected cleanly.

## Phase 2 - Hardware and runtime profiling

**Architecture coverage:** Diagram 3B and hardware feedback input layer.

Deliverables:

- CPU vendor/model, ISA features, cache hierarchy, core topology, page size, and NUMA discovery.
- Capability checks that distinguish CPU support from OS-enabled SIMD state.
- `perf stat` collection for cycles, instructions, branches, branch misses, cache references, cache misses, and elapsed time.
- Versioned hardware-profile and runtime-metrics JSON schemas.
- Permission-aware fallback when PMU counters are unavailable.

Acceptance gate:

1. No host-specific constants.
2. Profiles validate against schemas.
3. Unsupported counters are marked unavailable, never silently replaced.
4. Repeated benchmark measurements include warm-up and variance.

## Phase 3 - Rule-based optimization engine

**Architecture coverage:** Diagram 3D, Diagram 8, and the optimization decision layer.

Deliverables:

- A typed decision record joining IR features, hardware profile, and selected pass pipeline.
- Real profitability rules for vectorization, unrolling, inlining, locality, and scheduling hints.
- Pass dependency/conflict validation.
- LLVM New Pass Manager integration with analysis preservation verified.
- Explainable logs: selected action, rejected actions, evidence, thresholds, and confidence.

Acceptance gate:

1. All decisions consume measured inputs.
2. Every custom pass has IR-level regression tests.
3. The engine can choose “no change.”
4. Optimized IR passes LLVM verification.

## Phase 4 - Correctness and benchmarking

**Architecture coverage:** Diagram 3E and the evaluation/storage layers.

Deliverables:

- Differential output comparison between baseline and HACOE binaries.
- Determinism checks, crash/timeout detection, and numeric-tolerance policies.
- Benchmark runner for microbenchmarks and representative compute, memory, branch, graph, and matrix workloads.
- Statistical comparison against `-O0`, `-O2`, `-O3`, and HACOE.
- Machine-readable results plus a human-readable report.

Acceptance gate:

1. Incorrect output blocks performance claims.
2. Results include repetitions, dispersion, and environment metadata.
3. HACOE reports regressions as prominently as improvements.
4. A reproducible benchmark command regenerates the report.

## Phase 5 - Dataset and supervised prediction

**Architecture coverage:** Diagram 3C feature, history, dataset, and ML layers.

Deliverables:

- Dataset assembled from Phase 1-4 records.
- Leakage-resistant train/validation/test split by program family.
- Baseline models for speedup and regression risk.
- Model registry containing feature schema, training data version, metrics, and artifact hash.
- Decision policy that falls back to Phase 3 rules under uncertainty.

Acceptance gate:

1. The model beats simple heuristic baselines on held-out program families.
2. Calibration and regression-risk metrics are reported.
3. Inference is reproducible and schema-compatible.
4. The model cannot bypass correctness validation.

## Phase 6 - Auto-tuning and reinforcement learning research

**Architecture coverage:** Diagram 3C reinforcement learning layer and Diagram 9 feedback loop.

Deliverables:

- Sandboxed pass-sequence search with strict compile/run budgets.
- State, action, reward, and termination definitions.
- Offline replay before live exploration.
- Multi-objective reward covering speed, code size, compile time, energy when available, and failure penalties.
- Comparison against random search, Bayesian optimization, and fixed `-O3`.

Acceptance gate:

1. Search never mutates production/default policy automatically.
2. Results outperform non-RL baselines under equal budgets.
3. Failed compilations, timeouts, and incorrect outputs receive explicit penalties.
4. Every learned policy is versioned and reversible.

## Phase 7 - Code generation, deployment, and research platform

**Architecture coverage:** Diagram 3F, Diagram 6, Diagram 7, Diagram 10, and reporting/storage layers.

Deliverables:

- Target-aware backend/linker orchestration without reimplementing LLVM code generation.
- Reproducible container or environment definition.
- Benchmark/result storage with provenance and migration strategy.
- Research dashboard for experiments, comparisons, regressions, and artifacts.
- Documentation, threat model, release process, and public reproducibility package.

Acceptance gate:

1. A new researcher can reproduce a published experiment.
2. All results trace to source revision, toolchain, hardware profile, policy, and raw metrics.
3. Deployment secrets are isolated from experiment artifacts.
4. A tagged release includes build, tests, benchmarks, and methodology.

## Immediate Phase 0 backlog

1. Add the root build and toolchain checks.
2. Replace the hard-coded hardware profiler output with measured data.
3. Replace the simulated pass ratio with a shared feature/decision contract.
4. Upgrade `scripts/hwcc.py` into a validated CLI pipeline.
5. Add Python orchestration tests and LLVM smoke tests.
6. Add CI and exact setup instructions.
