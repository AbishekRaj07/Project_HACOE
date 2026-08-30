#include "hacoe/IRFeatures.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/SHA256.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <array>

namespace hacoe {
namespace {

std::string toHex(const std::array<std::uint8_t, 32> &digest) {
    constexpr char HexDigits[] = "0123456789abcdef";
    std::string result;
    result.reserve(digest.size() * 2);
    for (const std::uint8_t byte : digest) {
        result.push_back(HexDigits[byte >> 4U]);
        result.push_back(HexDigits[byte & 0x0FU]);
    }
    return result;
}

std::string canonicalModuleHash(const llvm::Module &module) {
    std::string canonicalIr;
    llvm::raw_string_ostream stream(canonicalIr);
    module.print(stream, nullptr);
    stream.flush();

    llvm::SHA256 hasher;
    hasher.update(llvm::StringRef(canonicalIr));
    return toHex(hasher.final());
}

void countLoopTree(
    const llvm::Loop &loop,
    const std::uint64_t depth,
    std::uint64_t &loopCount,
    std::uint64_t &maximumDepth
) {
    ++loopCount;
    maximumDepth = std::max(maximumDepth, depth);
    for (const llvm::Loop *subLoop : loop.getSubLoops()) {
        countLoopTree(*subLoop, depth + 1, loopCount, maximumDepth);
    }
}

bool isVectorInstruction(const llvm::Instruction &instruction) {
    if (instruction.getType()->isVectorTy()) {
        return true;
    }
    return llvm::any_of(instruction.operands(), [](const llvm::Use &operand) {
        return operand->getType()->isVectorTy();
    });
}

FunctionFeatures analyzeFunction(llvm::Function &function) {
    FunctionFeatures features;
    features.name = function.getName().str();

    llvm::DominatorTree dominatorTree(function);
    llvm::LoopInfo loopInfo(dominatorTree);
    for (const llvm::Loop *loop : loopInfo) {
        countLoopTree(*loop, 1, features.loops, features.maximumLoopDepth);
    }

    for (const llvm::BasicBlock &block : function) {
        ++features.basicBlocks;
        features.cfgEdges += llvm::succ_size(&block);

        for (const llvm::Instruction &instruction : block) {
            ++features.instructions;
            ++features.opcodeHistogram[instruction.getOpcodeName()];

            if (const auto *branch = llvm::dyn_cast<llvm::BranchInst>(&instruction)) {
                if (branch->isConditional()) {
                    ++features.conditionalBranches;
                } else {
                    ++features.unconditionalBranches;
                }
            } else if (llvm::isa<llvm::SwitchInst>(instruction)) {
                ++features.switches;
            }

            if (const auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction)) {
                ++features.calls;
                if (call->getCalledFunction() == nullptr && !call->isInlineAsm()) {
                    ++features.indirectCalls;
                }
            }

            features.loads += llvm::isa<llvm::LoadInst>(instruction);
            features.stores += llvm::isa<llvm::StoreInst>(instruction);
            features.atomics += llvm::isa<llvm::AtomicRMWInst>(instruction) ||
                llvm::isa<llvm::AtomicCmpXchgInst>(instruction);
            features.vectorInstructions += isVectorInstruction(instruction);
        }
    }

    return features;
}

llvm::json::Object functionToJson(const FunctionFeatures &features) {
    llvm::json::Object histogram;
    for (const auto &[opcode, count] : features.opcodeHistogram) {
        histogram[opcode] = static_cast<std::int64_t>(count);
    }

    return llvm::json::Object{
        {"name", features.name},
        {"basic_blocks", static_cast<std::int64_t>(features.basicBlocks)},
        {"cfg_edges", static_cast<std::int64_t>(features.cfgEdges)},
        {"instructions", static_cast<std::int64_t>(features.instructions)},
        {"loops", static_cast<std::int64_t>(features.loops)},
        {"maximum_loop_depth", static_cast<std::int64_t>(features.maximumLoopDepth)},
        {"conditional_branches", static_cast<std::int64_t>(features.conditionalBranches)},
        {"unconditional_branches", static_cast<std::int64_t>(features.unconditionalBranches)},
        {"switches", static_cast<std::int64_t>(features.switches)},
        {"calls", static_cast<std::int64_t>(features.calls)},
        {"indirect_calls", static_cast<std::int64_t>(features.indirectCalls)},
        {"loads", static_cast<std::int64_t>(features.loads)},
        {"stores", static_cast<std::int64_t>(features.stores)},
        {"atomics", static_cast<std::int64_t>(features.atomics)},
        {"vector_instructions", static_cast<std::int64_t>(features.vectorInstructions)},
        {"opcode_histogram", std::move(histogram)},
    };
}

} // namespace

ModuleFeatures analyzeModule(llvm::Module &module) {
    ModuleFeatures features;
    features.moduleSha256 = canonicalModuleHash(module);
    features.sourceFile = module.getSourceFileName();
    features.targetTriple = module.getTargetTriple();
    features.dataLayout = module.getDataLayoutStr();

    for (llvm::Function &function : module) {
        if (function.isDeclaration()) {
            ++features.declarations;
            continue;
        }
        ++features.definedFunctions;
        FunctionFeatures functionFeatures = analyzeFunction(function);
        features.basicBlocks += functionFeatures.basicBlocks;
        features.instructions += functionFeatures.instructions;
        features.functions.push_back(std::move(functionFeatures));
    }

    std::sort(
        features.functions.begin(),
        features.functions.end(),
        [](const FunctionFeatures &left, const FunctionFeatures &right) {
            return left.name < right.name;
        }
    );
    return features;
}

llvm::json::Value toJson(const ModuleFeatures &features) {
    llvm::json::Array functions;
    for (const FunctionFeatures &function : features.functions) {
        functions.push_back(functionToJson(function));
    }

    llvm::json::Object summary{
        {"defined_functions", static_cast<std::int64_t>(features.definedFunctions)},
        {"declarations", static_cast<std::int64_t>(features.declarations)},
        {"basic_blocks", static_cast<std::int64_t>(features.basicBlocks)},
        {"instructions", static_cast<std::int64_t>(features.instructions)},
    };

    return llvm::json::Object{
        {"schema_version", IRFeatureSchemaVersion},
        {"module_sha256", features.moduleSha256},
        {"source_file", features.sourceFile},
        {"target_triple", features.targetTriple},
        {"data_layout", features.dataLayout},
        {"summary", std::move(summary)},
        {"functions", std::move(functions)},
    };
}

void writeJson(llvm::raw_ostream &output, const ModuleFeatures &features) {
    llvm::json::Value value = toJson(features);
    output << llvm::formatv("{0:2}", value) << '\n';
}

} // namespace hacoe
