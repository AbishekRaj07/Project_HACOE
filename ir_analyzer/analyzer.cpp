#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <string>

using namespace llvm;

int main(int argc, char **argv) {
    if (argc < 2) {
        errs() << "Usage: " << argv[0] << " <path_to_ir_file.ll>\n";
        return 1;
    }

    StringRef InputFilename = argv[1];
    LLVMContext Context;
    SMDiagnostic Err;

    // Load the LLVM IR file
    std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
    
    if (!M) {
        Err.print(argv[0], errs());
        return 1;
    }

    outs() << "========================================\n";
    outs() << "   HACOE Static IR Topology Analyzer    \n";
    outs() << "========================================\n";
    outs() << "Target Module: " << M->getModuleIdentifier() << "\n\n";

    int total_functions = 0;
    int total_blocks = 0;
    int total_instructions = 0;

    // Iterate over all functions in the module
    for (Function &F : *M) {
        if (F.isDeclaration()) continue; // Skip external functions (like printf)

        total_functions++;
        outs() << "[Function] " << F.getName() << "\n";
        
        int func_blocks = 0;
        int func_insts = 0;

        // Iterate over all Basic Blocks in the function
        for (BasicBlock &BB : F) {
            func_blocks++;
            total_blocks++;
            
            // Iterate over all Instructions in the Basic Block
            for (Instruction &I : BB) {
                func_insts++;
                total_instructions++;
            }
        }
        
        outs() << "  -> Basic Blocks: " << func_blocks << "\n";
        outs() << "  -> Instructions: " << func_insts << "\n";
    }

    outs() << "----------------------------------------\n";
    outs() << "Global Topology Summary:\n";
    outs() << "Total Functions Identified : " << total_functions << "\n";
    outs() << "Total Basic Blocks (Nodes) : " << total_blocks << "\n";
    outs() << "Total Instructions (Edges) : " << total_instructions << "\n";
    outs() << "========================================\n";

    return 0;
}
