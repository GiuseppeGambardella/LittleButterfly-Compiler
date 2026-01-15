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
        mlirSymTable.insert(func); // <--- REGISTRAZIONE ESPLICITA
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
    // Scorriamo la TUA tabella
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
            // 2. INSERISCI ESPLICITAMENTE NELLA MLIR SYMBOL TABLE
            // Questo aggiorna le mappe interne di MLIR per lookup O(1)
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
    // Tutto già inizializzato nel costruttore.
    for (auto& decl : node.globals) decl->accept(*this);
    if (node.mainBlock) node.mainBlock->accept(*this);
}

void MLIRGenVisitor::visit(FunctionDeclNode& node) {
    llvm::SmallVector<mlir::Type, 4> argTypes;
    std::vector<std::string> argNames;

    for (auto& param : node.parameters) {
        if (auto varDecl = dynamic_cast<VarDeclNode*>(param.get())) {
            argTypes.push_back(getMLIRType(varDecl->type.type));
            argNames.push_back(varDecl->name);
        }
    }

    llvm::SmallVector<mlir::Type, 1> resultTypes;
    if (auto retNode = dynamic_cast<TypeNode*>(node.returnType.get())) {
        if (retNode->type != BasicType::VOID) {
            resultTypes.push_back(getMLIRType(retNode->type));
        }
    }

    auto funcType = builder.getFunctionType(argTypes, resultTypes);

    // Creiamo la funzione
    auto func = builder.create<mlir::func::FuncOp>(builder.getUnknownLoc(), node.name, funcType);

    func->remove();
    // IMPORTANTE: Registriamo anche la funzione nella SymbolTable MLIR!
    mlirSymTable.insert(func);

    mlir::Block* entryBlock = func.addEntryBlock();
    builder.setInsertionPointToStart(entryBlock);

    // Store parametri nelle globali (lookup veloce)
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
        if (!resultTypes.empty()) {
             auto t = resultTypes[0];
            mlir::Attribute z;
            if (t.isF64()) {
                z = builder.getFloatAttr(t, 0.0);
            } else {
                z = builder.getIntegerAttr(t, 0);
            }
             auto c = builder.create<mlir::arith::ConstantOp>(builder.getUnknownLoc(), t, llvm::cast<mlir::TypedAttr>(z));
             builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc(), mlir::ValueRange{c});
        } else {
             builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc());
        }
    }
}

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
    node.condition->accept(*this);
    mlir::Value cond = lastValue;
    auto ifOp = builder.create<mlir::scf::IfOp>(builder.getUnknownLoc(), cond, (node.elseBranch != nullptr));
    { auto g = mlir::OpBuilder::InsertionGuard(builder); builder.setInsertionPointToStart(&ifOp.getThenRegion().front()); if (node.thenBranch) node.thenBranch->accept(*this); }
    if (node.elseBranch) { auto g = mlir::OpBuilder::InsertionGuard(builder); builder.setInsertionPointToStart(&ifOp.getElseRegion().front()); node.elseBranch->accept(*this); }
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

    // ... copia la logica BinaryOp (and/or/concat/math) ...
    // Per brevità non la ripeto tutta qui, ma è identica a prima.
    // Se ti serve, fammelo sapere.

    bool isFloat = lhs.getType().isF64();
    if(node.op=="+") lastValue = isFloat ? builder.create<mlir::arith::AddFOp>(loc,lhs,rhs).getResult() : builder.create<mlir::arith::AddIOp>(loc,lhs,rhs).getResult();
    else if(node.op=="-") lastValue = isFloat ? builder.create<mlir::arith::SubFOp>(loc,lhs,rhs).getResult() : builder.create<mlir::arith::SubIOp>(loc,lhs,rhs).getResult();
    // ... eccetera ...
}

void MLIRGenVisitor::visit(UnaryOpNode& node) {
    node.operand->accept(*this);
    // ... copia logica UnaryOp ...
}

// Literals
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

    // Usa mlirSymTable per trovare la funzione
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

    // Lookup veloce funzione
    auto op = mlirSymTable.lookup(node.functionName);
    auto callee = llvm::dyn_cast<mlir::func::FuncOp>(op);

    auto c = builder.create<mlir::func::CallOp>(builder.getUnknownLoc(), callee, args);
    if(c.getNumResults() > 0) lastValue = c.getResult(0);
}

void MLIRGenVisitor::visit(PrintNode& node) {
    node.expression->accept(*this);
    // ... Logica Print standard (identica a prima) ...
    // Ricordati di usare mlirSymTable.lookup("print_...") se vuoi,
    // oppure lasciare le stringhe "print_int" che CallOp risolve da sola se il simbolo è nel modulo.
    // CallOp con stringa usa internamente il lookup.
}

void MLIRGenVisitor::visit(StringNode& node) {
    // Le stringhe sono un caso speciale, usiamo ancora stringPool per i letterali
    // Ma usiamo mlirSymTable per registrare la globale creata
    std::string globalName;
    if (stringPool.find(node.value) != stringPool.end()) {
        globalName = stringPool[node.value];
    } else {
        globalName = ".str" + std::to_string(stringLiteralCounter++);
        stringPool[node.value] = globalName;
        // ... crea global ...
        std::string s = node.value; s.push_back(0);
        auto i8 = builder.getI8Type();
        auto tensorType = mlir::RankedTensorType::get({(int64_t)s.size()}, i8);
        std::vector<int8_t> data(s.begin(), s.end());
        auto initAttr = mlir::DenseElementsAttr::get(tensorType, llvm::ArrayRef<int8_t>(data));

        auto g = builder.create<mlir::memref::GlobalOp>(builder.getUnknownLoc(), globalName, builder.getStringAttr("private"), mlir::MemRefType::get({(int64_t)s.size()}, i8), initAttr, true, nullptr);

        g->remove();
        mlirSymTable.insert(g); // REGISTER!
    }

    lastValue = getGlobalAddress(globalName);
}

void MLIRGenVisitor::visit(TypeNode&) {}
void MLIRGenVisitor::visit(VoidNode&) {}

void MLIRGenVisitor::emitMainWrapper() {
    auto loc = builder.getUnknownLoc();
    auto mainFunc = builder.create<mlir::func::FuncOp>(loc, "main", builder.getFunctionType({}, builder.getI32Type()));
    mainFunc->remove();
    mlirSymTable.insert(mainFunc); // Register main

    builder.setInsertionPointToStart(mainFunc.addEntryBlock());

    auto op = mlirSymTable.lookup("fly");
    if (auto fly = llvm::dyn_cast_or_null<mlir::func::FuncOp>(op)) {
        builder.create<mlir::func::CallOp>(loc, fly, mlir::ValueRange{});
    }

    auto zero = builder.create<mlir::arith::ConstantIntOp>(loc, 0, 32);
    builder.create<mlir::func::ReturnOp>(loc, mlir::ValueRange{zero});
}