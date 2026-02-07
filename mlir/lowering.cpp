#include "lowering.hpp"

#include <stdexcept>

#include "mlir/Pass/PassManager.h"
#include "mlir/IR/DialectRegistry.h"

#include "mlir/Transforms/Passes.h"
#include "mlir/Conversion/Passes.h"
#include "mlir/Conversion/SCFToControlFlow/SCFToControlFlow.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"

#include "mlir/Target/LLVMIR/Export.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"




// This function registers the necessary MLIR dialects into the provided MLIR context.
// It ensures that the context is aware of the dialects used during the lowering process,
// including Builtin, Arith, MemRef, SCF, and LLVM dialects.
void lowering::registerDialects(mlir::MLIRContext &context) {
    mlir::DialectRegistry registry;

    // Register the required dialects
    registry.insert<
        mlir::BuiltinDialect,
        mlir::arith::ArithDialect,
        mlir::memref::MemRefDialect,
        mlir::scf::SCFDialect,
        mlir::LLVM::LLVMDialect
    >();

    // Append the registry to the context and load all dialects
    context.appendDialectRegistry(registry);
    context.loadAllAvailableDialects();

    // Register translations for Builtin and LLVM dialects
    mlir::registerBuiltinDialectTranslation(context);
    mlir::registerLLVMDialectTranslation(context);
}



// This function performs the lowering from high-level MLIR dialects to the LLVM dialect.
// It applies a series of transformation passes to convert constructs from various dialects
// (like Arith, MemRef, SCF, Func) into their LLVM equivalents.
// If any pass fails, it dumps the module for debugging and throws a runtime error.
void lowering::lowerToLLVMDialect(mlir::ModuleOp module) {
    mlir::MLIRContext *ctx = module.getContext();

    mlir::PassManager pm(ctx);
    pm.enableVerifier(true);

    // ───────────────────────
    // Optimization MLIR
    // ───────────────────────
    pm.addPass(mlir::createCanonicalizerPass());
    pm.addPass(mlir::createCSEPass());
    pm.addPass(mlir::createLoopInvariantCodeMotionPass());
    pm.addPass(mlir::createSCCPPass());

    // ───────────────────────
    // ALL → LLVM
    // ───────────────────────
    pm.addPass(mlir::createArithToLLVMConversionPass());
    pm.addPass(mlir::createFinalizeMemRefToLLVMConversionPass());
    pm.addPass(mlir::createConvertFuncToLLVMPass());
    pm.addPass(mlir::createSCFToControlFlowPass());
    pm.addPass(mlir::createConvertControlFlowToLLVMPass());

    // ───────────────────────
    // Cleanup
    // ───────────────────────
    pm.addPass(mlir::createReconcileUnrealizedCastsPass());

    if (mlir::failed(pm.run(module))) {
        module.dump();
        throw std::runtime_error("Errore nel lowering MLIR -> LLVM");
    }
}


// This function translates an MLIR module into LLVM IR.
// It uses the MLIR to LLVM IR translation utilities.
// If the translation fails, it dumps the MLIR module for debugging
// and throws a runtime error.
std::unique_ptr<llvm::Module> lowering::translateToLLVMIR(mlir::ModuleOp module, llvm::LLVMContext &llvmContext) {

    // Translate the MLIR module to LLVM IR
    auto llvmModule =
        mlir::translateModuleToLLVMIR(module, llvmContext);

    // Check if the translation was successful
    if (!llvmModule) {
        module.dump();
        throw std::runtime_error(
            "translateModuleToLLVMIR failed"
        );
    }

    return llvmModule;
}


