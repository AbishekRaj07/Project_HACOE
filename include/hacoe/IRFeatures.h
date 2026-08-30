#pragma once

#include "llvm/Support/JSON.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace llvm {
class Module;
class raw_ostream;
} // namespace llvm

namespace hacoe {

inline constexpr char IRFeatureSchemaVersion[] = "1.0.0";

struct FunctionFeatures {
    std::string name;
    std::uint64_t basicBlocks = 0;
    std::uint64_t cfgEdges = 0;
    std::uint64_t instructions = 0;
    std::uint64_t loops = 0;
    std::uint64_t maximumLoopDepth = 0;
    std::uint64_t conditionalBranches = 0;
    std::uint64_t unconditionalBranches = 0;
    std::uint64_t switches = 0;
    std::uint64_t calls = 0;
    std::uint64_t indirectCalls = 0;
    std::uint64_t loads = 0;
    std::uint64_t stores = 0;
    std::uint64_t atomics = 0;
    std::uint64_t vectorInstructions = 0;
    std::map<std::string, std::uint64_t> opcodeHistogram;
};

struct ModuleFeatures {
    std::string moduleSha256;
    std::string sourceFile;
    std::string targetTriple;
    std::string dataLayout;
    std::uint64_t definedFunctions = 0;
    std::uint64_t declarations = 0;
    std::uint64_t basicBlocks = 0;
    std::uint64_t instructions = 0;
    std::vector<FunctionFeatures> functions;
};

ModuleFeatures analyzeModule(llvm::Module &module);
llvm::json::Value toJson(const ModuleFeatures &features);
void writeJson(llvm::raw_ostream &output, const ModuleFeatures &features);

} // namespace hacoe
