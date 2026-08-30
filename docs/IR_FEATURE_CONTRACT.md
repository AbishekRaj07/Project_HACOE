# LLVM IR Feature Contract v1

Phase 1 introduces a single deterministic feature contract shared by the standalone `hacoe-ir-analyzer` and the `FeatureExtractor` LLVM pass plugin.

## Design guarantees

- **Versioned:** every document carries `schema_version: "1.0.0"`.
- **Deterministic:** functions and opcode keys have a stable lexical order, and repeated analysis of identical LLVM modules produces identical JSON.
- **Canonical identity:** `module_sha256` hashes LLVM's canonical textual representation, not the input path or incidental whitespace of the source file.
- **Non-mutating:** analysis preserves all LLVM IR.
- **Machine validated:** [`schemas/ir-features-v1.schema.json`](../schemas/ir-features-v1.schema.json) is the authoritative external schema.
- **Producer parity:** CI requires the CLI and pass plugin to emit the same document for a golden fixture.

## Module fields

| Field | Meaning |
|---|---|
| `module_sha256` | SHA-256 of canonical LLVM IR |
| `source_file` | Source identity embedded inside the module |
| `target_triple` | LLVM target triple; may be empty when absent from IR |
| `data_layout` | LLVM data-layout string; may be empty when absent |
| `summary` | Defined/declaration counts and aggregate block/instruction totals |
| `functions` | Lexically ordered records for defined functions only |

## Per-function fields

Each function records basic blocks, CFG edges, total instructions, natural loops, maximum loop nesting, conditional and unconditional branches, switches, direct/indirect calls, loads, stores, atomic operations, vector-typed instructions, and a complete LLVM opcode histogram.

These are static IR properties. They are not runtime costs, cache behavior, arithmetic intensity, or proof that a transformation is profitable. Later phases may join this document with measured hardware and runtime profiles, but they must not silently reinterpret these counts.

## CLI

```bash
./build/hacoe-ir-analyzer input.ll
./build/hacoe-ir-analyzer input.bc --output features.json
```

Invalid IR returns a non-zero status and an LLVM diagnostic. Output-file failures use a separate non-zero status.

## LLVM pass plugin

```bash
opt \
  -load-pass-plugin build/llvm_passes/FeatureExtractor.so \
  -passes=extract-features \
  -disable-output \
  input.ll 2>features.json
```
