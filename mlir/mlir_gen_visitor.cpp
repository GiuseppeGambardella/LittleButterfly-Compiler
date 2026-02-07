#include "mlir_gen_visitor.hpp"
#include <iostream>

// ==========================================================
// CONSTRUCTOR AND INITIALIZATION
// ==========================================================

MLIRGenVisitor::MLIRGenVisitor(mlir::MLIRContext& ctx, SymbolTable& table)
    : context(ctx),
      symTable(table),
      builder(&ctx),
      // 1.Creates main module: it is the root container for all generated code.
      theModule(mlir::ModuleOp::create(builder.getUnknownLoc())),
      // 2. Initialization of MLIR Symbol Table linked to the module.
      // This table will manage visibility of functions and global variables in the IR.
      mlirSymTable(theModule)
{

    // Insert cursor (builder) is set to the end of the module body.
    // This is where new functions and globals will be added.
    builder.setInsertionPointToEnd(theModule.getBody());

    // 3. Runtime function declarations (e.g., print, read) that the language can call.
    declareRuntimeFunctions();

    // 4. This step creates MLIR global variables for each global variable found in the SymbolTable.
    initializeGlobalsFromSymbolTable();
}

// Utility function to print the generated MLIR code to standard output.
void MLIRGenVisitor::dump() {
    theModule.print(llvm::outs());
}


// Function used to declare runtime functions in the MLIR module.
// These functions are implemented in runtime.cpp and provide I/O and utility operations.
void MLIRGenVisitor::declareRuntimeFunctions() {
    auto loc = builder.getUnknownLoc();


    // Defining primitive MLIR types for function signatures.
    auto i32 = builder.getI32Type();
    auto f64 = builder.getF64Type();
    auto i8 = builder.getI8Type();
    auto i1 = builder.getI1Type(); // boolean type (1 bit).
    // MemRef dinamico di i8 rappresenta una stringa o array di char.
    auto stringType = mlir::MemRefType::get({mlir::ShapedType::kDynamic}, i8); // dynamic memref of i8 represents an array of chars

    // Helper function to create a function declaration and insert it into the MLIR symbol table.
    auto declare = [&](const std::string& name, mlir::FunctionType type) {
        auto func = builder.create<mlir::func::FuncOp>(loc, name, type);
        func.setPrivate();
        func->remove();
        mlirSymTable.insert(func);
    };

    declare("random", builder.getFunctionType({i32}, {i32}));

    // --- Output ---
    declare("print_int", builder.getFunctionType({i32}, {}));
    declare("print_double", builder.getFunctionType({f64}, {}));
    declare("print_char", builder.getFunctionType({i8}, {}));
    declare("print_string", builder.getFunctionType({stringType}, {}));

    // --- Input ---
    declare("read_int", builder.getFunctionType({}, {i32}));
    declare("read_double", builder.getFunctionType({}, {f64}));
    declare("read_char", builder.getFunctionType({}, {i8}));
    declare("read_string", builder.getFunctionType({}, {stringType}));

    // --- Conversione e Stringhe ---
    declare("to_string_int", builder.getFunctionType({i32}, {stringType}));
    declare("to_string_double", builder.getFunctionType({f64}, {stringType}));
    declare("to_string_bool", builder.getFunctionType({i1}, {stringType}));
    declare("to_string_char", builder.getFunctionType({i8}, {stringType}));
    declare("to_string_string", builder.getFunctionType({stringType}, {stringType}));
    declare("concat_strings", builder.getFunctionType({stringType, stringType}, {stringType}));
    declare("strcmp_strings", builder.getFunctionType({stringType, stringType}, {i32}));
}


// Helper to convert your language types (BasicType) to MLIR types.
// This is essential for defining function signatures and variable types in MLIR.
mlir::Type MLIRGenVisitor::getMLIRType(BasicType type) {
    switch (type) {
        case BasicType::INT: return builder.getI32Type();
        case BasicType::DOUBLE: return builder.getF64Type();
        case BasicType::BOOL: return builder.getI1Type();
        case BasicType::CHAR: return builder.getI8Type();
        case BasicType::STRING:
            return mlir::MemRefType::get(
                {mlir::ShapedType::kDynamic},
                builder.getI8Type()
            );
        case BasicType::VOID: return builder.getNoneType();
        default: return builder.getI32Type();
    }
}

// ==========================================================
// CORE: LOOKUP TRAMITE MLIR::SYMBOLTABLE
// ==========================================================


// This function initializes MLIR global variables based on the symbols found
// in the provided SymbolTable during semantic analysis.
// It creates 'memref.global' operations for each global variable
// and inserts them into the MLIR SymbolTable for later reference.
// This ensures that global variables are properly represented in the generated MLIR code.
void MLIRGenVisitor::initializeGlobalsFromSymbolTable() {

    // Iteriamo su tutti i simboli trovati durante l'analisi semantica.
    for (const auto& pair : symTable.getTable()) {
        const std::string& name = pair.first;
        const SymbolInfo& info = pair.second;

        // Only variables are handled here (functions are handled in visit(ProgramNode)).
        if (!info.isFunction) {

            //if it exists already in the MLIR symbol table, skip to avoid duplicates
            if (mlirSymTable.lookup(name)) continue;


            if (info.type == BasicType::STRING) {
                continue; // global strings are handled differently (in stringEnv)
            }



            mlir::Type type = getMLIRType(info.type);
            // Creating MemRef type for the global variable.
            auto memRefType = mlir::MemRefType::get({}, type);

            // Creation of the 'memref.global' operation.
            // This operation allocates static memory for the global variable.
            auto globalOp = builder.create<mlir::memref::GlobalOp>(
                builder.getUnknownLoc(),
                name,
                builder.getStringAttr("private"),
                memRefType,
                mlir::Attribute(), // initial value null
                false,
                nullptr
            );


            globalOp->remove();

            // Insert the global variable into the MLIR symbol table for future lookups.
            mlirSymTable.insert(globalOp);
        }
    }
}

// Helper to retrieve the address of a global variable by its name.
// This function looks up the 'memref.global' operation in the MLIR SymbolTable
// and creates a 'memref.get_global' operation to obtain the memory address.
// This address can then be used for load/store operations.
// If the variable is not found or is not a global, it returns nullptr.
// @param name The name of the global variable.
mlir::Value MLIRGenVisitor::getGlobalAddress(const std::string& name) {
    // Lookup the global variable in the MLIR SymbolTable.
    auto op = mlirSymTable.lookup(name);

    if (!op) {
        std::cerr << "ERROR: Symbol '" << name << "' not found in mlir::SymbolTable\n";
        return nullptr;
    }

    auto globalOp = llvm::dyn_cast<mlir::memref::GlobalOp>(op);
    if (!globalOp) {
        std::cerr << "ERROR: Symbol '" << name << "'; exists but it's not GlobalOp\n";
        return nullptr;
    }


    // Creation of 'memref.get_global' operation.
    // This operation retrieves the address of the global variable.
    return builder.create<mlir::memref::GetGlobalOp>(
        builder.getUnknownLoc(),
        globalOp.getType(),
        name
    );
}

// ==========================================================
// VISITORS
// ==========================================================

void MLIRGenVisitor::visit(ProgramNode& node) {
    // -------------------------------------------------------------
    // PASS 1: Declares all functions (prototypes)
    // -------------------------------------------------------------

    // A. User-defined functions (found in global declarations)
    for (auto& decl : node.globals) {
        if (auto funcDecl = dynamic_cast<FunctionDeclNode*>(decl.get())) {

            // arguments list building
            llvm::SmallVector<mlir::Type, 4> argTypes;
            for (auto& param : funcDecl->parameters) {
                if (auto varDecl = dynamic_cast<VarDeclNode*>(param.get())) {
                    argTypes.push_back(getMLIRType(varDecl->type.type));
                }
            }

            // return types list building
            llvm::SmallVector<mlir::Type, 1> resultTypes;
            if (auto retNode = dynamic_cast<TypeNode*>(funcDecl->returnType.get())) {
                if (retNode->type != BasicType::VOID) {
                    resultTypes.push_back(getMLIRType(retNode->type));
                }
            }

            // Function and insert into the symbol table
            auto funcType = builder.getFunctionType(argTypes, resultTypes);
            auto func = builder.create<mlir::func::FuncOp>(builder.getUnknownLoc(), funcDecl->name, funcType);

            func->remove();
            mlirSymTable.insert(func);
        }
    }

    // B. Fly function (main entry point)
    if (node.mainBlock) {

        // 'fly' is treated as a special FunctionDeclNode
        if (auto flyDecl = dynamic_cast<FunctionDeclNode*>(node.mainBlock.get())) {

            llvm::SmallVector<mlir::Type, 4> argTypes;
            for (auto& param : flyDecl->parameters) {
                if (auto varDecl = dynamic_cast<VarDeclNode*>(param.get())) {
                    argTypes.push_back(getMLIRType(varDecl->type.type));
                }
            }

            llvm::SmallVector<mlir::Type, 1> resultTypes;
            if (auto retNode = dynamic_cast<TypeNode*>(flyDecl->returnType.get())) {
                if (retNode->type != BasicType::VOID) {
                    resultTypes.push_back(getMLIRType(retNode->type));
                }
            }

            auto funcType = builder.getFunctionType(argTypes, resultTypes);
            auto func = builder.create<mlir::func::FuncOp>(builder.getUnknownLoc(), flyDecl->name, funcType);

            func->remove();
            mlirSymTable.insert(func);
        }
    }

    // -------------------------------------------------------------
    // PASS 2: Function bodies generation
    // -------------------------------------------------------------

    for (auto& decl : node.globals) {
        decl->accept(*this);
    }

    if (node.mainBlock) node.mainBlock->accept(*this);
}

void MLIRGenVisitor::visit(FunctionDeclNode& node) {

    // Retrieve the function from the MLIR SymbolTable
    auto op = mlirSymTable.lookup(node.name);
    auto func = llvm::dyn_cast_or_null<mlir::func::FuncOp>(op);

    if (!func) {
        std::cerr << "FATAL ERROR: Function '" << node.name << "' missing.\n";
        return;
    }

    // Retrieving argument names for parameter binding
    std::vector<std::string> argNames;
    for (auto& param : node.parameters) {
        if (auto varDecl = dynamic_cast<VarDeclNode*>(param.get())) {
            argNames.push_back(varDecl->name);
        }
    }

    // avoiding regeneration (if already generated)
    if (!func.getBlocks().empty()) return;

    // Entry Block creation and setting insertion point
    mlir::Block* entryBlock = func.addEntryBlock();
    builder.setInsertionPointToStart(entryBlock);

    // Arguments binding
    for (size_t i = 0; i < argNames.size(); ++i) {
        mlir::Value argValue = entryBlock->getArgument(i);
        const auto* info = symTable.lookup(argNames[i]);
        if (!info) continue;

        if (info->type == BasicType::STRING) {
            // string arguments are handled in stringEnv
            stringEnv[argNames[i]] = argValue;
            continue;
        }

        // for other types, store the argument value into the global variable
        mlir::Value globalPtr = getGlobalAddress(argNames[i]);
        if (!globalPtr) continue;

        builder.create<mlir::memref::StoreOp>(
            builder.getUnknownLoc(), argValue, globalPtr
        );

    }


    // Function body generation
    if (node.body) node.body->accept(*this);


    // Check for terminator in the current block
    mlir::Block* currentBlock = builder.getBlock();
    bool hasTerminator = !currentBlock->empty() && currentBlock->back().hasTrait<mlir::OpTrait::IsTerminator>();

    if (!hasTerminator) {
        // if no terminator, we need to add a return
        auto resultTypes = func.getFunctionType().getResults();
        if (!resultTypes.empty()) {
             auto t = resultTypes[0];
             mlir::Attribute z;
             if (t.isF64()) z = builder.getFloatAttr(t, 0.0);
             else z = builder.getIntegerAttr(t, 0);

             auto c = builder.create<mlir::arith::ConstantOp>(builder.getUnknownLoc(), t, llvm::cast<mlir::TypedAttr>(z));
             builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc(), mlir::ValueRange{c});
        } else {
             builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc());
        }
    }
}

// ==========================================================
// OTHER VISITORS
// ==========================================================

void MLIRGenVisitor::visit(VarDeclNode& node) {
    if (!node.initializer) return;

    node.initializer->accept(*this);
    const auto* info = symTable.lookup(node.name);
    if (!info) return;

    if (info->type == BasicType::STRING) {
        stringEnv[node.name] = lastValue;
        return;
    }

    mlir::Value globalPtr = getGlobalAddress(node.name);
    if (!globalPtr) return;

    builder.create<mlir::memref::StoreOp>(
        builder.getUnknownLoc(), lastValue, globalPtr
    );

}

void MLIRGenVisitor::visit(VariableNode& node) {

    const auto info = symTable.lookup(node.name);

    // String variables are handled differently
    if (info->type == BasicType::STRING) {
        auto it = stringEnv.find(node.name);
        if (it == stringEnv.end()) {
            std::cerr << "ERROR: string '" << node.name << "' not initialized\n";
        } else {
            lastValue = it->second;
        }
        return;
    }

    // Other variable types
    mlir::Value address = getGlobalAddress(node.name);

    if (address) {
        lastValue = builder.create<mlir::memref::LoadOp>(builder.getUnknownLoc(), address);
    } else {
        // Fallback: constant zero value (if variable not found)
        auto i32 = builder.getI32Type();
        auto zero = builder.getIntegerAttr(i32, 0);
        lastValue = builder.create<mlir::arith::ConstantOp>(builder.getUnknownLoc(), i32, zero);
    }
}

void MLIRGenVisitor::visit(AssignmentNode& node) {
    node.value->accept(*this);
    const auto* info = symTable.lookup(node.variableName);
    if (!info) return;

    if (info->type == BasicType::STRING) {
        stringEnv[node.variableName] = lastValue;
        return;
    }

    mlir::Value address = getGlobalAddress(node.variableName);
    if (!address) return;

    builder.create<mlir::memref::StoreOp>(
        builder.getUnknownLoc(), lastValue, address
    );

}

void MLIRGenVisitor::visit(BlockNode& node) {
    // Visit all statements in the block
    for (auto& s : node.statements) s->accept(*this);
}

void MLIRGenVisitor::visit(ReturnNode& node) {

    bool hasReturnValue = node.value && !dynamic_cast<VoidNode*>(node.value.get());

    // Genera l'istruzione di ritorno dalla funzione (func.return)
    if (hasReturnValue) {
        node.value->accept(*this);
        builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc(), lastValue);
    }
    else {
        builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc());
    }
}

void MLIRGenVisitor::visit(IfNode& node) {

    // 1. Condition evaluation
    node.condition->accept(*this);
    mlir::Value cond = lastValue;

    mlir::Block* currentBlock = builder.getBlock();
    mlir::Region* region = currentBlock->getParent();

    // 2. Creates blocks for THEN, ELSE and MERGE
    mlir::Block* thenBlock = builder.createBlock(region);
    mlir::Block* elseBlock = nullptr;
    if (node.elseBranch) {
        elseBlock = builder.createBlock(region);
    }
    mlir::Block* mergeBlock = builder.createBlock(region); // Blocco di uscita

    // 3. Generates branching based on condition
    builder.setInsertionPointToEnd(currentBlock);
    if (elseBlock) {
        builder.create<mlir::cf::CondBranchOp>(builder.getUnknownLoc(), cond, thenBlock, elseBlock);
    } else {
        builder.create<mlir::cf::CondBranchOp>(builder.getUnknownLoc(), cond, thenBlock, mergeBlock);
    }

    // 4. Code gen for THEN branch
    builder.setInsertionPointToStart(thenBlock);
    node.thenBranch->accept(*this);

    if (builder.getBlock()->empty() || !builder.getBlock()->back().hasTrait<mlir::OpTrait::IsTerminator>()) {
        builder.create<mlir::cf::BranchOp>(builder.getUnknownLoc(), mergeBlock);
    }

    // 5. Code gen for ELSE branch (if present)
    if (elseBlock) {
        builder.setInsertionPointToStart(elseBlock);
        node.elseBranch->accept(*this);

        if (builder.getBlock()->empty() || !builder.getBlock()->back().hasTrait<mlir::OpTrait::IsTerminator>()) {
            builder.create<mlir::cf::BranchOp>(builder.getUnknownLoc(), mergeBlock);
        }
    }

    // 6. Gen merge block
    builder.setInsertionPointToStart(mergeBlock);
}

void MLIRGenVisitor::visit(LoopNode& node) {

    // Retrieve current block and region
    mlir::Block* currentBlock = builder.getBlock();
    mlir::Region* region = currentBlock->getParent();

    // 1. Block creation: Header, Body, Exit
    mlir::Block* headerBlock = builder.createBlock(region);
    mlir::Block* bodyBlock = builder.createBlock(region);
    mlir::Block* exitBlock = builder.createBlock(region);

    // 2. Jump to Header
    builder.setInsertionPointToEnd(currentBlock);
    builder.create<mlir::cf::BranchOp>(builder.getUnknownLoc(), headerBlock);

    // --- HEADER:  ---
    builder.setInsertionPointToStart(headerBlock);
    node.condition->accept(*this);
    builder.create<mlir::cf::CondBranchOp>(builder.getUnknownLoc(), lastValue, bodyBlock, exitBlock);

    // --- BODY: ---
    builder.setInsertionPointToStart(bodyBlock);
    if (node.body) node.body->accept(*this);

    // 3. Back-edge: Jump to Header
    mlir::Block* bodyEndBlock = builder.getBlock();
    if (bodyEndBlock->empty() || !bodyEndBlock->back().hasTrait<mlir::OpTrait::IsTerminator>()) {
        builder.create<mlir::cf::BranchOp>(builder.getUnknownLoc(), headerBlock);
    }

    // 4. Set the builder insertion point to the Exit block for subsequent code generation
    builder.setInsertionPointToStart(exitBlock);
}

void MLIRGenVisitor::visit(BinaryOpNode& node) {
    node.left->accept(*this); mlir::Value lhs = lastValue;
    node.right->accept(*this); mlir::Value rhs = lastValue;
    auto loc = builder.getUnknownLoc();
    auto lhsType = lhs.getType();

    auto lhsMem = llvm::dyn_cast<mlir::MemRefType>(lhs.getType());
    auto rhsMem = llvm::dyn_cast<mlir::MemRefType>(rhs.getType());
    bool isStringCmp =
        lhsMem && rhsMem &&
        lhsMem.getElementType().isInteger(8) &&
        rhsMem.getElementType().isInteger(8);

    if (isStringCmp && (node.op == "==" || node.op == "<>" ||
    node.op == "<"  || node.op == ">"  ||
    node.op == "<=" || node.op == ">=")) {
        auto op = mlirSymTable.lookup("strcmp_strings");
        auto strcmpFn = llvm::dyn_cast<mlir::func::FuncOp>(op);
        if (!strcmpFn) {
            std::cerr << "ERRORE: strcmp_strings not found\n";
            return;
        }

        // call strcmp_strings(lhs, rhs)
        auto call = builder.create<mlir::func::CallOp>(
            loc, strcmpFn, mlir::ValueRange{lhs, rhs}
        );

        mlir::Value cmp = call.getResult(0); // i32
        auto zero = builder.create<mlir::arith::ConstantIntOp>(loc, 0, 32);

        if (node.op == "==") {
            lastValue = builder.create<mlir::arith::CmpIOp>(
                loc, mlir::arith::CmpIPredicate::eq, cmp, zero);
        }
        else if (node.op == "<>") {
            lastValue = builder.create<mlir::arith::CmpIOp>(
                loc, mlir::arith::CmpIPredicate::ne, cmp, zero);
        }
        else if (node.op == "<") {
            lastValue = builder.create<mlir::arith::CmpIOp>(
                loc, mlir::arith::CmpIPredicate::slt, cmp, zero);
        }
        else if (node.op == ">") {
            lastValue = builder.create<mlir::arith::CmpIOp>(
                loc, mlir::arith::CmpIPredicate::sgt, cmp, zero);
        }
        else if (node.op == "<=") {
            lastValue = builder.create<mlir::arith::CmpIOp>(
                loc, mlir::arith::CmpIPredicate::sle, cmp, zero);
        }
        else if (node.op == ">=") {
            lastValue = builder.create<mlir::arith::CmpIOp>(
                loc, mlir::arith::CmpIPredicate::sge, cmp, zero);
        }

        return;
    }


    // Logic operators AND / OR
    // Convert i32 (0/1) to i1 (bool) if necessary
    if (node.op == "and" || node.op == "or") {
        if (!lhsType.isInteger(1)) {
            auto zero = builder.create<mlir::arith::ConstantIntOp>(loc, 0, 32);
            lhs = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::ne, lhs, zero);
        }
        auto rhsType = rhs.getType();
        if (!rhsType.isInteger(1)) {
            auto zero = builder.create<mlir::arith::ConstantIntOp>(loc, 0, 32);
            rhs = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::ne, rhs, zero);
        }
        if (node.op == "and") lastValue = builder.create<mlir::arith::AndIOp>(loc, lhs, rhs);
        else lastValue = builder.create<mlir::arith::OrIOp>(loc, lhs, rhs);
        return;
    }

    // String concat
    if (node.op == "&") {

        // lambda to convert any type to string using runtime functions
        auto convertToString = [&](mlir::Value v, mlir::Type t) -> mlir::Value {
            std::string fnName;
            if (t.isInteger(32)) fnName = "to_string_int";
            else if (t.isInteger(1)) fnName = "to_string_bool";
            else if (t.isF64()) fnName = "to_string_double";
            else if (t.isInteger(8)) fnName = "to_string_char";
            else if (auto memTy = llvm::dyn_cast<mlir::MemRefType>(t)) {
                if (memTy.getElementType().isInteger(8)) {
                    auto dynStrType = mlir::MemRefType::get({mlir::ShapedType::kDynamic}, builder.getI8Type());
                    if (memTy != dynStrType) v = builder.create<mlir::memref::CastOp>(loc, dynStrType, v);
                    fnName = "to_string_string";
                } else return nullptr;
            } else return nullptr;

            auto op = mlirSymTable.lookup(fnName);
            auto callee = llvm::dyn_cast<mlir::func::FuncOp>(op);
            if (!callee) return nullptr;
            return builder.create<mlir::func::CallOp>(loc, callee, mlir::ValueRange{v}).getResult(0);
        };

        mlir::Value lhsStr = convertToString(lhs, lhs.getType());
        mlir::Value rhsStr = convertToString(rhs, rhs.getType());

        // Calls concat_strings
        if (lhsStr && rhsStr) {
            auto op = mlirSymTable.lookup("concat_strings");
            auto concatFn = llvm::dyn_cast<mlir::func::FuncOp>(op);
            if (concatFn) {
                lastValue = builder.create<mlir::func::CallOp>(loc, concatFn, mlir::ValueRange{lhsStr, rhsStr}).getResult(0);
            }
        }
        return;
    }

    // Arith op
    bool isFloat = lhsType.isF64();
    if(node.op=="+") lastValue = isFloat ? builder.create<mlir::arith::AddFOp>(loc,lhs,rhs).getResult() : builder.create<mlir::arith::AddIOp>(loc,lhs,rhs).getResult();
    else if(node.op=="-") lastValue = isFloat ? builder.create<mlir::arith::SubFOp>(loc,lhs,rhs).getResult() : builder.create<mlir::arith::SubIOp>(loc,lhs,rhs).getResult();
    else if(node.op=="*") lastValue = isFloat ? builder.create<mlir::arith::MulFOp>(loc,lhs,rhs).getResult() : builder.create<mlir::arith::MulIOp>(loc,lhs,rhs).getResult();
    else if(node.op=="/") lastValue = isFloat ? builder.create<mlir::arith::DivFOp>(loc,lhs,rhs).getResult() : builder.create<mlir::arith::DivSIOp>(loc,lhs,rhs).getResult();
    else if (node.op == "==") {

        if(isFloat) lastValue = builder.create<mlir::arith::CmpFOp>(loc, mlir::arith::CmpFPredicate::OEQ, lhs, rhs);
        else lastValue = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::eq, lhs, rhs);
    }
    else if (node.op == "<>") {
        if(isFloat) lastValue = builder.create<mlir::arith::CmpFOp>(loc, mlir::arith::CmpFPredicate::ONE, lhs, rhs);
        else lastValue = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::ne, lhs, rhs);
    }
    else if (node.op == "<") {
        if(isFloat) lastValue = builder.create<mlir::arith::CmpFOp>(loc, mlir::arith::CmpFPredicate::OLT, lhs, rhs);
        else lastValue = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::slt, lhs, rhs);
    }
    else if (node.op == ">") {
        if(isFloat) lastValue = builder.create<mlir::arith::CmpFOp>(loc, mlir::arith::CmpFPredicate::OGT, lhs, rhs);
        else lastValue = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::sgt, lhs, rhs);
    }
    else if (node.op == "<=") {
        if(isFloat) lastValue = builder.create<mlir::arith::CmpFOp>(loc, mlir::arith::CmpFPredicate::OLE, lhs, rhs);
        else lastValue = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::sle, lhs, rhs);
    }
    else if (node.op == ">=") {
        if(isFloat) lastValue = builder.create<mlir::arith::CmpFOp>(loc, mlir::arith::CmpFPredicate::OGE, lhs, rhs);
        else lastValue = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::sge, lhs, rhs);
    }
}

void MLIRGenVisitor::visit(UnaryOpNode& node) {
    node.operand->accept(*this);
    auto val = lastValue;
    auto loc = builder.getUnknownLoc();

    // unary minus, logical not, random
    if (node.op == "-") {
        if (val.getType().isF64()) {
            auto zero = builder.create<mlir::arith::ConstantOp>(loc, builder.getF64Type(), builder.getFloatAttr(builder.getF64Type(), 0.0));
            lastValue = builder.create<mlir::arith::SubFOp>(loc, zero, val);
        } else {
            auto zero = builder.create<mlir::arith::ConstantOp>(loc, val.getType(), builder.getIntegerAttr(val.getType(), 0));
            lastValue = builder.create<mlir::arith::SubIOp>(loc, zero, val);
        }
    } else if (node.op == "!") {
        auto one = builder.create<mlir::arith::ConstantOp>(loc, builder.getI1Type(), builder.getIntegerAttr(builder.getI1Type(), 1));
        lastValue = builder.create<mlir::arith::XOrIOp>(loc, val, one);
    }
    else if (node.op == "?") {
        // call to random function
        auto op = mlirSymTable.lookup("random");
        auto callee = llvm::dyn_cast<mlir::func::FuncOp>(op);
        auto call = builder.create<mlir::func::CallOp>(loc, callee, mlir::ValueRange{val});
        lastValue = call.getResult(0);
    }
}

// Literal nodes visitors
void MLIRGenVisitor::visit(NumberNode& node) { lastValue = builder.create<mlir::arith::ConstantIntOp>(builder.getUnknownLoc(), node.value, 32); }
void MLIRGenVisitor::visit(RealNode& node) { lastValue = builder.create<mlir::arith::ConstantOp>(builder.getUnknownLoc(), builder.getF64Type(), builder.getFloatAttr(builder.getF64Type(), node.value)); }
void MLIRGenVisitor::visit(BooleanNode& node) { lastValue = builder.create<mlir::arith::ConstantIntOp>(builder.getUnknownLoc(), node.value, 1); }
void MLIRGenVisitor::visit(CharNode& node) { lastValue = builder.create<mlir::arith::ConstantIntOp>(builder.getUnknownLoc(), node.value, 8); }

void MLIRGenVisitor::visit(ReadNode& node) {
    auto* var = dynamic_cast<VariableNode*>(node.variable.get());
    if(!var) return;
    const auto* info = symTable.lookup(var->name);

    // Choose the appropriate read function based on variable type
    std::string fn = "read_int";
    if(info->type == BasicType::DOUBLE) fn = "read_double";
    else if(info->type == BasicType::CHAR) fn = "read_char";
    else if (info->type == BasicType::STRING) fn = "read_string";

    auto op = mlirSymTable.lookup(fn);
    auto callee = llvm::dyn_cast<mlir::func::FuncOp>(op);
    auto call = builder.create<mlir::func::CallOp>(builder.getUnknownLoc(), callee, mlir::ValueRange{});
    mlir::Value val = call.getResult(0);

    // if boolean then convert int to bool
    if (info->type == BasicType::BOOL) {
         auto zero = builder.create<mlir::arith::ConstantIntOp>(builder.getUnknownLoc(), 0, 32);
         val = builder.create<mlir::arith::CmpIOp>(builder.getUnknownLoc(), mlir::arith::CmpIPredicate::ne, val, zero);
    }

    if (info->type == BasicType::STRING) {
        stringEnv[var->name] = val; // <-- store in stringEnv
        return;
    }

    // Store input value into the variable
    mlir::Value addr = getGlobalAddress(var->name);
    if (!addr) return;

    builder.create<mlir::memref::StoreOp>(
        builder.getUnknownLoc(), val, addr
    );

}

void MLIRGenVisitor::visit(FunctionCallNode& node) {
    std::vector<mlir::Value> args;
    for(auto& a : node.arguments) { a->accept(*this); args.push_back(lastValue); }

    // Do the function call
    auto op = mlirSymTable.lookup(node.functionName);
    auto callee = llvm::dyn_cast<mlir::func::FuncOp>(op);

    auto c = builder.create<mlir::func::CallOp>(builder.getUnknownLoc(), callee, args);
    if(c.getNumResults() > 0) lastValue = c.getResult(0);
}

void MLIRGenVisitor::visit(PrintNode& node) {
    node.expression->accept(*this);
    mlir::Value arg = lastValue;
    mlir::Type type = arg.getType();
    auto loc = builder.getUnknownLoc();

    if (auto memRefType = mlir::dyn_cast<mlir::MemRefType>(type)) {
        if (memRefType.getElementType().isInteger(8) && memRefType.getRank() > 0) {
            auto dynamicStringType = mlir::MemRefType::get({mlir::ShapedType::kDynamic}, builder.getI8Type());
            arg = builder.create<mlir::memref::CastOp>(loc, dynamicStringType, arg);
            builder.create<mlir::func::CallOp>(loc, "print_string", mlir::TypeRange{}, mlir::ValueRange{arg});
            return;
        }

        // if it's not a string, load the value
        arg = builder.create<mlir::memref::LoadOp>(loc, arg);
        type = arg.getType();
    }

    // function selection based on type
    std::string funcName = "print_int";
    if (type.isF64()) funcName = "print_double";
    else if (type.isInteger(8)) funcName = "print_char";
    else if (type.isInteger(1)) {
        arg = builder.create<mlir::arith::ExtUIOp>(loc, builder.getI32Type(), arg);
    }
    builder.create<mlir::func::CallOp>(loc, funcName, mlir::TypeRange{}, mlir::ValueRange{arg});
}

void MLIRGenVisitor::visit(StringNode& node) {
    std::string globalName;

    // if it exists in the pool, reuse it
    if (stringPool.find(node.value) != stringPool.end()) {
        globalName = stringPool[node.value];
    } else {
        // if not found, create a new global string
        globalName = ".str" + std::to_string(stringLiteralCounter++);
        stringPool[node.value] = globalName;
        std::string s = node.value; s.push_back(0); // Terminatore null
        auto i8 = builder.getI8Type();
        auto tensorType = mlir::RankedTensorType::get({(int64_t)s.size()}, i8);
        std::vector<int8_t> data(s.begin(), s.end());
        auto initAttr = mlir::DenseElementsAttr::get(tensorType, llvm::ArrayRef<int8_t>(data));

        auto g = builder.create<mlir::memref::GlobalOp>(builder.getUnknownLoc(), globalName, builder.getStringAttr("private"), mlir::MemRefType::get({(int64_t)s.size()}, i8), initAttr, true, nullptr);

        g->remove();
        mlirSymTable.insert(g);
    }
    mlir::Value staticPtr = getGlobalAddress(globalName);

    // cast to memref<?xi8>
    auto dynamicStrType = mlir::MemRefType::get({mlir::ShapedType::kDynamic}, builder.getI8Type());
    lastValue = builder.create<mlir::memref::CastOp>(builder.getUnknownLoc(), dynamicStrType, staticPtr);
}

// Type nodes visitors (no-op)
void MLIRGenVisitor::visit(TypeNode&) {}
void MLIRGenVisitor::visit(VoidNode&) {}

// This function emits the standard C "main" function that serves as the entry point
// for the generated MLIR code. The "main" function calls the "fly" function
// which is treated as the main program body in this language.
// The "main" function returns an integer value of 0 to indicate successful execution.
// This wrapper is necessary to conform to the C runtime expectations.
void MLIRGenVisitor::emitMainWrapper() {
    auto loc = builder.getUnknownLoc();
    // creates 'main' function
    auto mainFunc = builder.create<mlir::func::FuncOp>(loc, "main", builder.getFunctionType({}, builder.getI32Type()));
    mainFunc->remove();
    mlirSymTable.insert(mainFunc);

    builder.setInsertionPointToStart(mainFunc.addEntryBlock());

    // calls 'fly' function
    auto op = mlirSymTable.lookup("fly");
    if (auto fly = llvm::dyn_cast_or_null<mlir::func::FuncOp>(op)) {
        builder.create<mlir::func::CallOp>(loc, fly, mlir::ValueRange{});
    }

    // Return 0 from main
    auto zero = builder.create<mlir::arith::ConstantIntOp>(loc, 0, 32);
    builder.create<mlir::func::ReturnOp>(loc, mlir::ValueRange{zero});
}