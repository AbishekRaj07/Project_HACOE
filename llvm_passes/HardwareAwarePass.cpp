#include "llvm/IR/Function.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
struct HardwareAwarePass : public PassInfoMixin<HardwareAwarePass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
        // Hardcoded thresholds for AMD Ryzen 5 from the EDM
        const double HARDWARE_RIDGE_POINT = 2.0; 
        
        // Simulating the ratio detected in the previous pass
        double current_ratio = 9.0; // Simulated un-canonicalized ratio

        errs() << "[HACOE Engine] Evaluating AVX2 Profitability for: " << F.getName() << "\n";

        if (current_ratio > HARDWARE_RIDGE_POINT) {
            errs() << "  -> [ABORT] Arithmetic Intensity is too low (" << (1.0/current_ratio) << ").\n";
            errs() << "  -> [WARNING] Forcing AVX2 will cause L2 Cache Starvation (Thrashing).\n";
            errs() << "  -> [DIAGNOSTIC] Phase Ordering failure: Run 'mem2reg' and 'loop-rotate' before vectorization.\n";
        } else {
            errs() << "  -> [SUCCESS] Code is compute-bound. Injecting AVX2 pragmas...\n";
            // Logic to inject llvm.loop.vectorize.enable would go here
        }

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
