#include "hacoe/IRFeatures.h"

#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/raw_ostream.h"

namespace {

class FeatureExtractorPass :
    public llvm::PassInfoMixin<FeatureExtractorPass> {
public:
    llvm::PreservedAnalyses run(
        llvm::Module &module,
        llvm::ModuleAnalysisManager &
    ) {
        hacoe::writeJson(llvm::errs(), hacoe::analyzeModule(module));
        return llvm::PreservedAnalyses::all();
    }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "HACOEFeatureExtractor",
        hacoe::IRFeatureSchemaVersion,
        [](llvm::PassBuilder &passBuilder) {
            passBuilder.registerPipelineParsingCallback(
                [](
                    llvm::StringRef name,
                    llvm::ModulePassManager &modulePassManager,
                    llvm::ArrayRef<llvm::PassBuilder::PipelineElement>
                ) {
                    if (name != "extract-features") {
                        return false;
                    }
                    modulePassManager.addPass(FeatureExtractorPass());
                    return true;
                }
            );
        },
    };
}
