#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>

using namespace llvm;

namespace {
struct HardwareAwarePass : public PassInfoMixin<HardwareAwarePass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
        // Phase 0 heuristic. Later phases will load a measured hardware profile.
        constexpr double MemoryToMathThreshold = 2.0;
        std::uint64_t memoryInstructions = 0;
        std::uint64_t mathInstructions = 0;

        for (BasicBlock &BB : F) {
            for (Instruction &I : BB) {
                if (isa<LoadInst>(I) || isa<StoreInst>(I)) {
                    ++memoryInstructions;
                } else if (isa<BinaryOperator>(I)) {
                    ++mathInstructions;
                }
            }
        }

        const double currentRatio = mathInstructions == 0
            ? static_cast<double>(memoryInstructions)
            : static_cast<double>(memoryInstructions) /
                  static_cast<double>(mathInstructions);

        errs() << "[HACOE Engine] Evaluating AVX2 Profitability for: " << F.getName() << "\n";

        errs() << "  -> Memory instructions: " << memoryInstructions << "\n";
        errs() << "  -> Math instructions: " << mathInstructions << "\n";
        errs() << "  -> Memory-to-math ratio: " << currentRatio << "\n";

        if (currentRatio > MemoryToMathThreshold) {
            errs() << "  -> [RECOMMENDATION] Keep the baseline pipeline; the function appears memory-heavy.\n";
        } else {
            errs() << "  -> [RECOMMENDATION] Evaluate vectorization profitability in Phase 3.\n";
        }

        // This pass currently reports a recommendation and does not mutate IR.
        return PreservedAnalyses::all();
    }
};
}

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "HardwareAwarePass", "v0.1",
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "hw-aware-opt") {
                        FPM.addPass(HardwareAwarePass());
                        return true;
                    }
                    return false;
                });
        }};
}
