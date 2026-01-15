#include "mlir_gen_visitor.hpp"
#include <iostream>

// ==========================================================
// COSTRUTTORE
// ==========================================================

MLIRGenVisitor::MLIRGenVisitor(mlir::MLIRContext& ctx, SymbolTable& table)
    : context(ctx),
      symTable(table),
      builder(&ctx),
      // 1. Creiamo il modulo
      theModule(mlir::ModuleOp::create(builder.getUnknownLoc())),
      // 2. Inizializziamo la MLIR Symbol Table legandola al modulo
      mlirSymTable(theModule)
{
    builder.setInsertionPointToEnd(theModule.getBody());

    // 3. Funzioni runtime
    declareRuntimeFunctions();

    // 4. SVUOTIAMO LA TUA TABELLA DENTRO QUELLA DI MLIR
    initializeGlobalsFromSymbolTable();
}

void MLIRGenVisitor::dump() {
    theModule.print(llvm::outs());
}

void MLIRGenVisitor::declareRuntimeFunctions() {
    auto loc = builder.getUnknownLoc();
    auto i32 = builder.getI32Type();
    auto f64 = builder.getF64Type();
    auto i8 = builder.getI8Type();
    auto i1 = builder.getI1Type();
    auto stringType = mlir::MemRefType::get({mlir::ShapedType::kDynamic}, i8);

    // Funzione helper per creare e inserire subito nella tabella MLIR
    auto declare = [&](const std::string& name, mlir::FunctionType type) {
        auto func = builder.create<mlir::func::FuncOp>(loc, name, type);
        func.setPrivate();
        func->remove();
        mlirSymTable.insert(func);
    };

    declare("print_int", builder.getFunctionType({i32}, {}));
    declare("print_double", builder.getFunctionType({f64}, {}));
    declare("print_char", builder.getFunctionType({i8}, {}));
    declare("print_string", builder.getFunctionType({stringType}, {}));

    declare("read_int", builder.getFunctionType({}, {i32}));
    declare("read_double", builder.getFunctionType({}, {f64}));
    declare("read_char", builder.getFunctionType({}, {i8}));

    declare("to_string_int", builder.getFunctionType({i32}, {stringType}));
    declare("to_string_double", builder.getFunctionType({f64}, {stringType}));
    declare("to_string_bool", builder.getFunctionType({i1}, {stringType}));
    declare("to_string_char", builder.getFunctionType({i8}, {stringType}));
    declare("to_string_string", builder.getFunctionType({stringType}, {stringType}));
    declare("concat_strings", builder.getFunctionType({stringType, stringType}, {stringType}));
}

mlir::Type MLIRGenVisitor::getMLIRType(BasicType type) {
    switch (type) {
        case BasicType::INT: return builder.getI32Type();
        case BasicType::DOUBLE: return builder.getF64Type();
        case BasicType::BOOL: return builder.getI1Type();
        case BasicType::CHAR: return builder.getI8Type();
        case BasicType::VOID: return builder.getNoneType();
        default: return builder.getI32Type();
    }
}

// ==========================================================
// CORE: POPOLAMENTO E LOOKUP TRAMITE MLIR::SYMBOLTABLE
// ==========================================================

void MLIRGenVisitor::initializeGlobalsFromSymbolTable() {
    // Scorriamo la tabella
    for (const auto& pair : symTable.getTable()) {
        const std::string& name = pair.first;
        const SymbolInfo& info = pair.second;

        // Gestiamo solo le variabili qui (le funzioni runtime sono già fatte, quelle utente nel visit)
        if (!info.isFunction) {

            // Check veloce sulla tabella MLIR
            if (mlirSymTable.lookup(name)) continue;

            mlir::Type type = getMLIRType(info.type);
            auto memRefType = mlir::MemRefType::get({}, type);


            // 1. Crea l'operazione (ancora non attaccata o appena attaccata)
            auto globalOp = builder.create<mlir::memref::GlobalOp>(
                builder.getUnknownLoc(),
                name,
                builder.getStringAttr("private"),
                memRefType,
                mlir::Attribute(),
                false,
                nullptr
            );

            globalOp->remove();
            // 2. INSERIRE ESPLICITAMENTE NELLA MLIR SYMBOL TABLE
            mlirSymTable.insert(globalOp);
        }
    }
}

mlir::Value MLIRGenVisitor::getGlobalAddress(const std::string& name) {
    // 1. LOOKUP EFFICIENTE TRAMITE MLIR::SYMBOLTABLE
    // Ritorna mlir::Operation*, non fa scan lineare del modulo.
    auto op = mlirSymTable.lookup(name);

    if (!op) {
        std::cerr << "ERRORE: Simbolo '" << name << "' non trovato in mlir::SymbolTable\n";
        return nullptr;
    }

    auto globalOp = llvm::dyn_cast<mlir::memref::GlobalOp>(op);
    if (!globalOp) {
        std::cerr << "ERRORE: Il simbolo '" << name << "' esiste ma non è una GlobalOp\n";
        return nullptr;
    }

    // 2. Genera l'accesso
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
    // PASSO 1: Dichiara TUTTI i prototipi (Globals + Fly)
    // -------------------------------------------------------------

    // A. Funzioni normali (in globals)
    for (auto& decl : node.globals) {
        if (auto funcDecl = dynamic_cast<FunctionDeclNode*>(decl.get())) {

            llvm::SmallVector<mlir::Type, 4> argTypes;
            for (auto& param : funcDecl->parameters) {
                if (auto varDecl = dynamic_cast<VarDeclNode*>(param.get())) {
                    argTypes.push_back(getMLIRType(varDecl->type.type));
                }
            }

            llvm::SmallVector<mlir::Type, 1> resultTypes;
            if (auto retNode = dynamic_cast<TypeNode*>(funcDecl->returnType.get())) {
                if (retNode->type != BasicType::VOID) {
                    resultTypes.push_back(getMLIRType(retNode->type));
                }
            }

            auto funcType = builder.getFunctionType(argTypes, resultTypes);
            auto func = builder.create<mlir::func::FuncOp>(builder.getUnknownLoc(), funcDecl->name, funcType);

            func->remove();
            mlirSymTable.insert(func);
        }
    }

    // B. Funzione FLY (MainBlock)
    if (node.mainBlock) {
        // Fly solitamente è una FunctionDeclNode speciale
        if (auto flyDecl = dynamic_cast<FunctionDeclNode*>(node.mainBlock.get())) {

            // Fly non ha argomenti
            llvm::SmallVector<mlir::Type, 4> argTypes;
            for (auto& param : flyDecl->parameters) {
                if (auto varDecl = dynamic_cast<VarDeclNode*>(param.get())) {
                    argTypes.push_back(getMLIRType(varDecl->type.type));
                }
            }

            // Fly solitamente è void
            llvm::SmallVector<mlir::Type, 1> resultTypes;
            if (auto retNode = dynamic_cast<TypeNode*>(flyDecl->returnType.get())) {
                if (retNode->type != BasicType::VOID) {
                    resultTypes.push_back(getMLIRType(retNode->type));
                }
            }

            auto funcType = builder.getFunctionType(argTypes, resultTypes);
            // Usiamo flyDecl->name che dovrebbe essere "fly"
            auto func = builder.create<mlir::func::FuncOp>(builder.getUnknownLoc(), flyDecl->name, funcType);

            func->remove();
            mlirSymTable.insert(func);
        }
    }

    // -------------------------------------------------------------
    // PASSO 2: Genera i corpi
    // -------------------------------------------------------------
    for (auto& decl : node.globals) {
        decl->accept(*this);
    }

    if (node.mainBlock) node.mainBlock->accept(*this);
}

void MLIRGenVisitor::visit(FunctionDeclNode& node) {
    // 1. RECUPERA la funzione (ora anche 'fly' sarà trovata qui!)
    auto op = mlirSymTable.lookup(node.name);
    auto func = llvm::dyn_cast_or_null<mlir::func::FuncOp>(op);

    if (!func) {
        std::cerr << "ERRORE INTERNO: Funzione '" << node.name << "' persa.\n";
        return;
    }

    std::vector<std::string> argNames;
    for (auto& param : node.parameters) {
        if (auto varDecl = dynamic_cast<VarDeclNode*>(param.get())) {
            argNames.push_back(varDecl->name);
        }
    }

    if (!func.getBlocks().empty()) return;

    mlir::Block* entryBlock = func.addEntryBlock();
    builder.setInsertionPointToStart(entryBlock);

    for (size_t i = 0; i < argNames.size(); ++i) {
        mlir::Value argValue = entryBlock->getArgument(i);
        mlir::Value globalPtr = getGlobalAddress(argNames[i]);
        if (globalPtr) {
            builder.create<mlir::memref::StoreOp>(builder.getUnknownLoc(), argValue, globalPtr);
        }
    }

    if (node.body) node.body->accept(*this);

    bool hasTerminator = !entryBlock->empty() && entryBlock->back().hasTrait<mlir::OpTrait::IsTerminator>();

    if (!hasTerminator) {
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
// ALTRI VISITOR (Standard)
// ==========================================================

void MLIRGenVisitor::visit(VarDeclNode& node) {
    if (node.initializer) {
        node.initializer->accept(*this);
        mlir::Value globalPtr = getGlobalAddress(node.name);
        if (globalPtr) {
            builder.create<mlir::memref::StoreOp>(builder.getUnknownLoc(), lastValue, globalPtr);
        }
    }
}

void MLIRGenVisitor::visit(VariableNode& node) {
    mlir::Value address = getGlobalAddress(node.name);
    if (address) {
        lastValue = builder.create<mlir::memref::LoadOp>(builder.getUnknownLoc(), address);
    } else {
        auto i32 = builder.getI32Type();
        auto zero = builder.getIntegerAttr(i32, 0);
        lastValue = builder.create<mlir::arith::ConstantOp>(builder.getUnknownLoc(), i32, zero);
    }
}

void MLIRGenVisitor::visit(AssignmentNode& node) {
    node.value->accept(*this);
    mlir::Value address = getGlobalAddress(node.variableName);
    if (address) {
        builder.create<mlir::memref::StoreOp>(builder.getUnknownLoc(), lastValue, address);
    }
}

void MLIRGenVisitor::visit(BlockNode& node) { for (auto& s : node.statements) s->accept(*this); }

void MLIRGenVisitor::visit(ReturnNode& node) {
    if (node.value) { node.value->accept(*this); builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc(), lastValue); }
    else builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc());
}

void MLIRGenVisitor::visit(IfNode& node) {
    // 1. Valuta la condizione
    node.condition->accept(*this);
    mlir::Value cond = lastValue;

    // Ottieni il blocco corrente e la regione della funzione
    mlir::Block* currentBlock = builder.getBlock();
    mlir::Region* region = currentBlock->getParent();

    // 2. Crea i nuovi blocchi per il Then, Else (opzionale) e Merge (uscita)
    mlir::Block* thenBlock = builder.createBlock(region);
    mlir::Block* elseBlock = nullptr;
    if (node.elseBranch) {
        elseBlock = builder.createBlock(region);
    }
    // Il blocco di continuazione (dove si uniscono i flussi)
    mlir::Block* mergeBlock = builder.createBlock(region);

    // 3. Crea il salto condizionale (CondBranch) dal blocco originale
    builder.setInsertionPointToEnd(currentBlock);
    if (elseBlock) {
        builder.create<mlir::cf::CondBranchOp>(builder.getUnknownLoc(), cond, thenBlock, elseBlock);
    } else {
        // Se non c'è else, salta al merge se la condizione è falsa
        builder.create<mlir::cf::CondBranchOp>(builder.getUnknownLoc(), cond, thenBlock, mergeBlock);
    }

    // --- GENERAZIONE RAMO THEN ---
    builder.setInsertionPointToStart(thenBlock);
    node.thenBranch->accept(*this);

    // Se il blocco non termina già con un return (o altro terminatore), salta al merge
    if (thenBlock->empty() || !thenBlock->back().hasTrait<mlir::OpTrait::IsTerminator>()) {
        builder.create<mlir::cf::BranchOp>(builder.getUnknownLoc(), mergeBlock);
    }

    // --- GENERAZIONE RAMO ELSE ---
    if (elseBlock) {
        builder.setInsertionPointToStart(elseBlock);
        node.elseBranch->accept(*this);

        if (elseBlock->empty() || !elseBlock->back().hasTrait<mlir::OpTrait::IsTerminator>()) {
            builder.create<mlir::cf::BranchOp>(builder.getUnknownLoc(), mergeBlock);
        }
    }

    // 4. Riposizionati sul blocco Merge per continuare a generare il resto del codice
    builder.setInsertionPointToStart(mergeBlock);
}

void MLIRGenVisitor::visit(LoopNode& node) {
    auto loc = builder.getUnknownLoc();
    auto whileOp = builder.create<mlir::scf::WhileOp>(loc, mlir::TypeRange{}, mlir::ValueRange{});
    mlir::Block* beforeBlock = builder.createBlock(&whileOp.getBefore());
    { node.condition->accept(*this); builder.create<mlir::scf::ConditionOp>(loc, lastValue, mlir::ValueRange{}); }
    mlir::Block* afterBlock = builder.createBlock(&whileOp.getAfter());
    { if (node.body) node.body->accept(*this); builder.create<mlir::scf::YieldOp>(loc); }
    builder.setInsertionPointAfter(whileOp);
}

void MLIRGenVisitor::visit(BinaryOpNode& node) {
    node.left->accept(*this); mlir::Value lhs = lastValue;
    node.right->accept(*this); mlir::Value rhs = lastValue;
    auto loc = builder.getUnknownLoc();
    auto lhsType = lhs.getType();

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

    if (node.op == "&") {
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

        if (lhsStr && rhsStr) {
            auto op = mlirSymTable.lookup("concat_strings");
            auto concatFn = llvm::dyn_cast<mlir::func::FuncOp>(op);
            if (concatFn) {
                lastValue = builder.create<mlir::func::CallOp>(loc, concatFn, mlir::ValueRange{lhsStr, rhsStr}).getResult(0);
            }
        }
        return;
    }

    bool isFloat = lhsType.isF64();
    if(node.op=="+") lastValue = isFloat ? builder.create<mlir::arith::AddFOp>(loc,lhs,rhs).getResult() : builder.create<mlir::arith::AddIOp>(loc,lhs,rhs).getResult();
    else if(node.op=="-") lastValue = isFloat ? builder.create<mlir::arith::SubFOp>(loc,lhs,rhs).getResult() : builder.create<mlir::arith::SubIOp>(loc,lhs,rhs).getResult();
    else if(node.op=="*") lastValue = isFloat ? builder.create<mlir::arith::MulFOp>(loc,lhs,rhs).getResult() : builder.create<mlir::arith::MulIOp>(loc,lhs,rhs).getResult();
    else if(node.op=="/") lastValue = isFloat ? builder.create<mlir::arith::DivFOp>(loc,lhs,rhs).getResult() : builder.create<mlir::arith::DivSIOp>(loc,lhs,rhs).getResult();
    else if (node.op == "==") {
        if(isFloat) lastValue = builder.create<mlir::arith::CmpFOp>(loc, mlir::arith::CmpFPredicate::OEQ, lhs, rhs);
        else lastValue = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::eq, lhs, rhs);
    }
    else if (node.op == "!=") {
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
}

void MLIRGenVisitor::visit(NumberNode& node) { lastValue = builder.create<mlir::arith::ConstantIntOp>(builder.getUnknownLoc(), node.value, 32); }
void MLIRGenVisitor::visit(RealNode& node) { lastValue = builder.create<mlir::arith::ConstantOp>(builder.getUnknownLoc(), builder.getF64Type(), builder.getFloatAttr(builder.getF64Type(), node.value)); }
void MLIRGenVisitor::visit(BooleanNode& node) { lastValue = builder.create<mlir::arith::ConstantIntOp>(builder.getUnknownLoc(), node.value, 1); }
void MLIRGenVisitor::visit(CharNode& node) { lastValue = builder.create<mlir::arith::ConstantIntOp>(builder.getUnknownLoc(), node.value, 8); }

void MLIRGenVisitor::visit(ReadNode& node) {
    auto* var = dynamic_cast<VariableNode*>(node.variable.get());
    if(!var) return;
    const auto* info = symTable.lookup(var->name);
    std::string fn = "read_int";
    if(info->type == BasicType::DOUBLE) fn = "read_double";
    else if(info->type == BasicType::CHAR) fn = "read_char";

    auto op = mlirSymTable.lookup(fn);
    auto callee = llvm::dyn_cast<mlir::func::FuncOp>(op);
    auto call = builder.create<mlir::func::CallOp>(builder.getUnknownLoc(), callee, mlir::ValueRange{});
    mlir::Value val = call.getResult(0);

    if (info->type == BasicType::BOOL) {
         auto zero = builder.create<mlir::arith::ConstantIntOp>(builder.getUnknownLoc(), 0, 32);
         val = builder.create<mlir::arith::CmpIOp>(builder.getUnknownLoc(), mlir::arith::CmpIPredicate::ne, val, zero);
    }

    mlir::Value addr = getGlobalAddress(var->name);
    builder.create<mlir::memref::StoreOp>(builder.getUnknownLoc(), val, addr);
}

void MLIRGenVisitor::visit(FunctionCallNode& node) {
    std::vector<mlir::Value> args;
    for(auto& a : node.arguments) { a->accept(*this); args.push_back(lastValue); }

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
        arg = builder.create<mlir::memref::LoadOp>(loc, arg);
        type = arg.getType();
    }

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
    if (stringPool.find(node.value) != stringPool.end()) {
        globalName = stringPool[node.value];
    } else {
        globalName = ".str" + std::to_string(stringLiteralCounter++);
        stringPool[node.value] = globalName;
        std::string s = node.value; s.push_back(0);
        auto i8 = builder.getI8Type();
        auto tensorType = mlir::RankedTensorType::get({(int64_t)s.size()}, i8);
        std::vector<int8_t> data(s.begin(), s.end());
        auto initAttr = mlir::DenseElementsAttr::get(tensorType, llvm::ArrayRef<int8_t>(data));

        auto g = builder.create<mlir::memref::GlobalOp>(builder.getUnknownLoc(), globalName, builder.getStringAttr("private"), mlir::MemRefType::get({(int64_t)s.size()}, i8), initAttr, true, nullptr);

        g->remove();
        mlirSymTable.insert(g);
    }
    lastValue = getGlobalAddress(globalName);
}

void MLIRGenVisitor::visit(TypeNode&) {}
void MLIRGenVisitor::visit(VoidNode&) {}

void MLIRGenVisitor::emitMainWrapper() {
    auto loc = builder.getUnknownLoc();
    auto mainFunc = builder.create<mlir::func::FuncOp>(loc, "main", builder.getFunctionType({}, builder.getI32Type()));
    mainFunc->remove();
    mlirSymTable.insert(mainFunc);

    builder.setInsertionPointToStart(mainFunc.addEntryBlock());

    auto op = mlirSymTable.lookup("fly");
    if (auto fly = llvm::dyn_cast_or_null<mlir::func::FuncOp>(op)) {
        builder.create<mlir::func::CallOp>(loc, fly, mlir::ValueRange{});
    }

    auto zero = builder.create<mlir::arith::ConstantIntOp>(loc, 0, 32);
    builder.create<mlir::func::ReturnOp>(loc, mlir::ValueRange{zero});
}