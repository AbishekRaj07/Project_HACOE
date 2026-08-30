#include "hacoe/IRFeatures.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <string>
#include <system_error>

namespace {

llvm::cl::opt<std::string> InputFilename(
    llvm::cl::Positional,
    llvm::cl::Required,
    llvm::cl::desc("<LLVM IR or bitcode file>")
);

llvm::cl::opt<std::string> OutputFilename(
    "output",
    llvm::cl::desc("Write JSON to this file ('-' means stdout)"),
    llvm::cl::value_desc("path"),
    llvm::cl::init("-")
);

} // namespace

int main(int argc, char **argv) {
    llvm::InitLLVM initialization(argc, argv);
    llvm::cl::ParseCommandLineOptions(
        argc,
        argv,
        "HACOE deterministic LLVM IR feature analyzer\n"
    );

    llvm::LLVMContext context;
    llvm::SMDiagnostic diagnostic;
    std::unique_ptr<llvm::Module> module =
        llvm::parseIRFile(InputFilename, diagnostic, context);
    if (!module) {
        diagnostic.print(argv[0], llvm::errs());
        return 2;
    }

    const hacoe::ModuleFeatures features = hacoe::analyzeModule(*module);
    if (OutputFilename == "-") {
        hacoe::writeJson(llvm::outs(), features);
        return 0;
    }

    std::error_code error;
    llvm::raw_fd_ostream output(OutputFilename, error, llvm::sys::fs::OF_Text);
    if (error) {
        llvm::errs() << "hacoe-ir-analyzer: cannot open '" << OutputFilename
                     << "': " << error.message() << '\n';
        return 3;
    }
    hacoe::writeJson(output, features);
    return 0;
}
