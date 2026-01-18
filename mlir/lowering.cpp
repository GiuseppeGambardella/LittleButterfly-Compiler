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





void lowering::registerDialects(mlir::MLIRContext &context) {
    mlir::DialectRegistry registry;

    registry.insert<
        mlir::BuiltinDialect,
        mlir::arith::ArithDialect,
        mlir::memref::MemRefDialect,
        mlir::scf::SCFDialect,
        mlir::LLVM::LLVMDialect
    >();

    context.appendDialectRegistry(registry);
    context.loadAllAvailableDialects();

    mlir::registerBuiltinDialectTranslation(context);
    mlir::registerLLVMDialectTranslation(context);
}




void lowering::lowerToLLVMDialect(mlir::ModuleOp module) {
    mlir::MLIRContext *ctx = module.getContext();

    mlir::PassManager pm(ctx);
    pm.enableVerifier(true);

    // ───────────────────────
    // 1️⃣ Optimization MLIR
    // ───────────────────────
    pm.addPass(mlir::createCanonicalizerPass());
    pm.addPass(mlir::createCSEPass());

    // (facoltativo ma consigliato)
    pm.addPass(mlir::createLoopInvariantCodeMotionPass());
    pm.addPass(mlir::createSCCPPass());

    // ───────────────────────
    // 2️⃣ SCF → Control Flow
    // ───────────────────────
   pm.addPass(mlir::createArithToLLVMConversionPass());

    // ───────────────────────
    // 3️⃣ ALL → LLVM
    // ───────────────────────
    pm.addPass(mlir::createArithToLLVMConversionPass());
    pm.addPass(mlir::createFinalizeMemRefToLLVMConversionPass());
    pm.addPass(mlir::createConvertFuncToLLVMPass());
    pm.addPass(mlir::createSCFToControlFlowPass());
    pm.addPass(mlir::createConvertControlFlowToLLVMPass());

    // ───────────────────────
    // 4️⃣ Cleanup
    // ───────────────────────
    pm.addPass(mlir::createReconcileUnrealizedCastsPass());

    if (mlir::failed(pm.run(module))) {
        module.dump();
        throw std::runtime_error("Errore nel lowering MLIR -> LLVM");
    }
}

std::unique_ptr<llvm::Module> lowering::translateToLLVMIR(mlir::ModuleOp module, llvm::LLVMContext &llvmContext) {

    auto llvmModule =
        mlir::translateModuleToLLVMIR(module, llvmContext);

    if (!llvmModule) {
        module.dump();
        throw std::runtime_error(
            "translateModuleToLLVMIR fallita"
        );
    }

    return llvmModule;
}


