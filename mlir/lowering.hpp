#pragma once

#include <memory>


#include "mlir/IR/MLIRContext.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "mlir/IR/BuiltinOps.h"

namespace lowering {

    /// Register all custom dialects used in the project
    void registerDialects(mlir::MLIRContext &context);

    /// Lowering:
    /// MLIR (func/arith/memref/scf) -> MLIR LLVM dialect
    void lowerToLLVMDialect(mlir::ModuleOp module);

    /// MLIR LLVM dialect -> llvm::Module
    std::unique_ptr<llvm::Module>
    translateToLLVMIR(mlir::ModuleOp module, llvm::LLVMContext &llvmContext);

}
