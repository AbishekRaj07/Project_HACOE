# Project HACOE: Hardware-Aware Compiler Optimization Engine

HACOE is an autonomous compiler infrastructure built on the LLVM New Pass Manager. It dynamically profiles host microarchitecture constraints and injects hardware-specific optimizations (like AVX2 vectorization and L2 cache tiling) into the middle-end optimizer.

## Features
* **Environment Discovery Module:** Queries POSIX limits and CPU microarchitecture (Cache lines, SIMD support).
* **Static CFG Feature Extractor:** Analyzes IR to calculate Memory-to-Math execution ratios.
* **Autonomous Compiler Driver:** Python orchestrator (`hwcc.py`) that handles frontend (Clang), custom ML/Rule passes (`opt`), and backend code generation (`llc`).
* **Hardware Validation:** Linux `perf` harness to capture cycle variations and cache misses on bare metal.
