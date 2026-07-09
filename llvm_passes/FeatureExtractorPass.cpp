#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
struct FeatureExtractorPass : public PassInfoMixin<FeatureExtractorPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
        int memory_inst = 0;
        int math_inst = 0;
        int total_inst = 0;

        for (BasicBlock &BB : F) {
            for (Instruction &I : BB) {
                total_inst++;
                if (isa<LoadInst>(&I) || isa<StoreInst>(&I)) {
                    memory_inst++;
                } else if (isa<BinaryOperator>(&I)) {
                    math_inst++;
                }
            }
        }

        // Avoid division by zero
        double ratio = (math_inst == 0) ? memory_inst : (double)memory_inst / math_inst;

        errs() << "--- HACOE CFG Tensor Extraction ---\n";
        errs() << "Function: " << F.getName() << "\n";
        errs() << "Total Instructions: " << total_inst << "\n";
        errs() << "Memory Instructions: " << memory_inst << "\n";
        errs() << "Math Instructions: " << math_inst << "\n";
        errs() << "Memory-to-Math Ratio: " << ratio << "\n";
        errs() << "-----------------------------------\n";

        return PreservedAnalyses::all();
    }
};
}

// Register the pass for the New Pass Manager
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "FeatureExtractorPass", "v0.1",
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "extract-features") {
                        FPM.addPass(FeatureExtractorPass());
                        return true;
                    }
                    return false;
                });
        }};
}
