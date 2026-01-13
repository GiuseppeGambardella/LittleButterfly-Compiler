#include "mlir_gen_visitor.hpp"

// ==========================================================
// COSTRUTTORE E HELPER DI BASE
// ==========================================================

MLIRGenVisitor::MLIRGenVisitor(mlir::MLIRContext& ctx)
    : context(ctx), builder(&ctx) {

    // Creazione del Modulo vuoto
    theModule = mlir::ModuleOp::create(builder.getUnknownLoc());

    // Impostiamo il punto di inserimento nel corpo del modulo
    builder.setInsertionPointToEnd(theModule.getBody());

    // Dichiariamo le funzioni esterne (es. print)
    declareRuntimeFunctions();
}

void MLIRGenVisitor::dump() {
    theModule.print(llvm::outs());
}

void MLIRGenVisitor::declareRuntimeFunctions() {
    // 1. print_int(i32) -> void
    auto i32 = builder.getI32Type();
    builder.create<mlir::func::FuncOp>(builder.getUnknownLoc(), "print_int",
        builder.getFunctionType({i32}, {})).setPrivate();

    // 2. print_double(f64) -> void
    auto f64 = builder.getF64Type();
    builder.create<mlir::func::FuncOp>(builder.getUnknownLoc(), "print_double",
        builder.getFunctionType({f64}, {})).setPrivate();

    // --- NUOVE AGGIUNTE PER CHAR E STRINGHE ---

    // 3. print_char(i8) -> void
    auto i8 = builder.getI8Type();
    builder.create<mlir::func::FuncOp>(builder.getUnknownLoc(), "print_char",
        builder.getFunctionType({i8}, {})).setPrivate();

    // 4. print_string(memref<?xi8>) -> void
    // Definiamo un array di char a dimensione dinamica
    auto stringType = mlir::MemRefType::get(
        {mlir::ShapedType::kDynamic}, // Dimensione ignota (?)
        i8                            // Tipo elemento (char)
    );
    builder.create<mlir::func::FuncOp>(builder.getUnknownLoc(), "print_string",
        builder.getFunctionType({stringType}, {})).setPrivate();
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
// HELPER PRIVATO: LOGICA SCOPE UNICO
// ==========================================================

mlir::Value MLIRGenVisitor::getGlobalAddress(const std::string& name) {
    // 1. Cerca nel Modulo (Scope Globale)
    auto globalOp = theModule.lookupSymbol<mlir::memref::GlobalOp>(name);

    if (!globalOp) {
        std::cerr << "Error: Global variable '" << name << "' not found!\n";
        return nullptr;
    }

    // 2. Genera il puntatore di accesso (GetGlobalOp)
    return builder.create<mlir::memref::GetGlobalOp>(
        builder.getUnknownLoc(),
        globalOp.getType(),
        name
    );
}

// ==========================================================
// VISITOR: STRUTTURA PROGRAMMA
// ==========================================================

void MLIRGenVisitor::visit(ProgramNode& node) {
    for (auto& decl : node.globals) decl->accept(*this);
    if (node.mainBlock) node.mainBlock->accept(*this);
}

void MLIRGenVisitor::visit(FunctionDeclNode& node) {
    // 1. Definiamo i tipi degli argomenti
    llvm::SmallVector<mlir::Type, 4> argTypes;
    std::vector<std::string> argNames;

    for (auto& param : node.parameters) {
        if (auto varDecl = dynamic_cast<VarDeclNode*>(param.get())) {
            argTypes.push_back(getMLIRType(varDecl->type.type));
            argNames.push_back(varDecl->name);
        }
    }

    // 2. Definiamo il tipo di ritorno
    llvm::SmallVector<mlir::Type, 1> resultTypes;
    if (auto retNode = dynamic_cast<TypeNode*>(node.returnType.get())) {
        if (retNode->type != BasicType::VOID) {
            resultTypes.push_back(getMLIRType(retNode->type));
        }
    }

    // 3. Creiamo la funzione
    auto funcType = builder.getFunctionType(argTypes, resultTypes);

    // Assicuriamoci di scrivere nel corpo del Modulo
    builder.setInsertionPointToEnd(theModule.getBody());
    auto func = builder.create<mlir::func::FuncOp>(builder.getUnknownLoc(), node.name, funcType);

    // 4. Creiamo il blocco di ingresso
    mlir::Block* entryBlock = func.addEntryBlock();
    builder.setInsertionPointToStart(entryBlock);

    // 5. TRUCCO "FAKE GLOBAL": Copiamo i parametri in variabili globali
    for (size_t i = 0; i < argNames.size(); ++i) {
        std::string paramName = argNames[i];
        mlir::Value argValue = entryBlock->getArgument(i);

        // A. Creazione/Check Variabile Globale (Saltiamo fuori dalla funzione temporaneamente)
        {
            auto guard = mlir::OpBuilder::InsertionGuard(builder);
            builder.setInsertionPointToEnd(theModule.getBody());

            if (!theModule.lookupSymbol<mlir::memref::GlobalOp>(paramName)) {
                auto type = argValue.getType();
                auto memRefType = mlir::MemRefType::get({}, type);

                mlir::Attribute zeroAttr;
                if (type.isF64()) zeroAttr = builder.getFloatAttr(type, 0.0);
                else zeroAttr = builder.getIntegerAttr(type, 0);

                builder.create<mlir::memref::GlobalOp>(
                    builder.getUnknownLoc(), paramName, builder.getStringAttr("private"),
                    memRefType, zeroAttr, false, nullptr
                );
            }
        }

        // B. Store del parametro nella globale
        mlir::Value globalPtr = getGlobalAddress(paramName);
        if (globalPtr) {
            builder.create<mlir::memref::StoreOp>(builder.getUnknownLoc(), argValue, globalPtr);
        }
    }

    // 6. Generiamo il corpo
    if (node.body) node.body->accept(*this);

    // 7. Return di sicurezza
    bool hasTerminator = !entryBlock->empty() &&
                         entryBlock->back().hasTrait<mlir::OpTrait::IsTerminator>();
    if (!hasTerminator) {
        builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc());
    }
}

void MLIRGenVisitor::visit(BlockNode& node) {
    for (auto& stmt : node.statements) stmt->accept(*this);
}

void MLIRGenVisitor::visit(ReturnNode& node) {
    if (node.value) {
        node.value->accept(*this);
        builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc(), lastValue);
    } else {
        builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc());
    }
}

// ==========================================================
// VISITOR: VARIABILI (Dichiarazione, Accesso, Assegnazione)
// ==========================================================

void MLIRGenVisitor::visit(VarDeclNode& node) {
    auto* currentBlock = builder.getBlock();
    auto currentPoint = builder.getInsertionPoint();

    // 1. Crea la variabile SEMPRE nel Modulo (Globale)
    builder.setInsertionPointToEnd(theModule.getBody());

    mlir::Type type = getMLIRType(node.type.type);
    auto memRefType = mlir::MemRefType::get({}, type);

    mlir::Attribute initAttr;
    if (type.isF64()) initAttr = builder.getFloatAttr(type, 0.0);
    else initAttr = builder.getIntegerAttr(type, 0);

    if (!theModule.lookupSymbol<mlir::memref::GlobalOp>(node.name)) {
        builder.create<mlir::memref::GlobalOp>(
            builder.getUnknownLoc(),
            node.name,
            builder.getStringAttr("private"),
            memRefType,
            initAttr,
            false,
            nullptr
        );
    }

    // 2. Torna nel contesto locale per l'inizializzazione
    if (currentBlock) {
        builder.setInsertionPoint(currentBlock, currentPoint);

        if (node.initializer) {
            node.initializer->accept(*this);
            mlir::Value valToStore = lastValue;

            mlir::Value globalPtr = getGlobalAddress(node.name);
            if (globalPtr) {
                builder.create<mlir::memref::StoreOp>(builder.getUnknownLoc(), valToStore, globalPtr);
            }
        }
    }
}

void MLIRGenVisitor::visit(VariableNode& node) {
    mlir::Value address = getGlobalAddress(node.name);
    if (address) {
        lastValue = builder.create<mlir::memref::LoadOp>(builder.getUnknownLoc(), address);
    } else {
        // Fallback per evitare crash
        auto i32 = builder.getI32Type();
        auto zero = builder.getIntegerAttr(i32, 0);
        lastValue = builder.create<mlir::arith::ConstantOp>(builder.getUnknownLoc(), i32, zero);
    }
}

void MLIRGenVisitor::visit(AssignmentNode& node) {
    node.value->accept(*this);
    mlir::Value valToStore = lastValue;

    mlir::Value address = getGlobalAddress(node.variableName);
    if (address) {
        builder.create<mlir::memref::StoreOp>(builder.getUnknownLoc(), valToStore, address);
    }
}

// ==========================================================
// VISITOR: FLUSSO DI CONTROLLO
// ==========================================================

void MLIRGenVisitor::visit(IfNode& node) {
    node.condition->accept(*this);
    mlir::Value cond = lastValue;

    auto ifOp = builder.create<mlir::scf::IfOp>(
        builder.getUnknownLoc(), cond, (node.elseBranch != nullptr));

    // Then Branch
    {
        auto guard = mlir::OpBuilder::InsertionGuard(builder);
        builder.setInsertionPointToStart(&ifOp.getThenRegion().front());
        if (node.thenBranch) node.thenBranch->accept(*this);
    }

    // Else Branch
    if (node.elseBranch) {
        auto guard = mlir::OpBuilder::InsertionGuard(builder);
        builder.setInsertionPointToStart(&ifOp.getElseRegion().front());
        node.elseBranch->accept(*this);
    }
}

void MLIRGenVisitor::visit(LoopNode& node) {
    auto loc = builder.getUnknownLoc();
    auto whileOp = builder.create<mlir::scf::WhileOp>(
        loc, mlir::TypeRange{}, mlir::ValueRange{}
    );

    // Before Region (Condition)
    mlir::Block* beforeBlock = builder.createBlock(&whileOp.getBefore());
    {
        node.condition->accept(*this);
        mlir::Value cond = lastValue;
        builder.create<mlir::scf::ConditionOp>(loc, cond, mlir::ValueRange{});
    }

    // After Region (Body)
    mlir::Block* afterBlock = builder.createBlock(&whileOp.getAfter());
    {
        if (node.body) node.body->accept(*this);
        builder.create<mlir::scf::YieldOp>(loc);
    }

    builder.setInsertionPointAfter(whileOp);
}

// ==========================================================
// VISITOR: OPERAZIONI
// ==========================================================

void MLIRGenVisitor::visit(BinaryOpNode& node) {
    node.left->accept(*this);
    mlir::Value lhs = lastValue;
    node.right->accept(*this);
    mlir::Value rhs = lastValue;
    auto loc = builder.getUnknownLoc();

    bool isFloat = lhs.getType().isF64();

    if (node.op == "+") {
        if (isFloat) lastValue = builder.create<mlir::arith::AddFOp>(loc, lhs, rhs);
        else lastValue = builder.create<mlir::arith::AddIOp>(loc, lhs, rhs);
    }
    else if (node.op == "-") {
        if (isFloat) lastValue = builder.create<mlir::arith::SubFOp>(loc, lhs, rhs);
        else lastValue = builder.create<mlir::arith::SubIOp>(loc, lhs, rhs);
    }
    else if (node.op == "*") {
        if (isFloat) lastValue = builder.create<mlir::arith::MulFOp>(loc, lhs, rhs);
        else lastValue = builder.create<mlir::arith::MulIOp>(loc, lhs, rhs);
    }
    else if (node.op == "/") {
        if (isFloat) lastValue = builder.create<mlir::arith::DivFOp>(loc, lhs, rhs);
        else lastValue = builder.create<mlir::arith::DivSIOp>(loc, lhs, rhs);
    }
    else if (node.op == "==" || node.op == "<" || node.op == ">") {
        if (isFloat) {
            mlir::arith::CmpFPredicate pred;
            if (node.op == "==") pred = mlir::arith::CmpFPredicate::OEQ;
            else if (node.op == "<") pred = mlir::arith::CmpFPredicate::OLT;
            else pred = mlir::arith::CmpFPredicate::OGT;
            lastValue = builder.create<mlir::arith::CmpFOp>(loc, pred, lhs, rhs);
        } else {
            mlir::arith::CmpIPredicate pred;
            if (node.op == "==") pred = mlir::arith::CmpIPredicate::eq;
            else if (node.op == "<") pred = mlir::arith::CmpIPredicate::slt;
            else pred = mlir::arith::CmpIPredicate::sgt;
            lastValue = builder.create<mlir::arith::CmpIOp>(loc, pred, lhs, rhs);
        }
    }
}

void MLIRGenVisitor::visit(UnaryOpNode& node) {
    node.operand->accept(*this);
    auto val = lastValue;
    auto loc = builder.getUnknownLoc();

    if (node.op == "-") {
        if (val.getType().isF64()) {
            auto type = builder.getF64Type();
            auto zeroAttr = builder.getFloatAttr(type, 0.0);
            auto zero = builder.create<mlir::arith::ConstantOp>(loc, type, llvm::cast<mlir::TypedAttr>(zeroAttr));
            lastValue = builder.create<mlir::arith::SubFOp>(loc, zero, val);
        } else {
            auto type = val.getType();
            auto zeroAttr = builder.getIntegerAttr(type, 0);
            auto zero = builder.create<mlir::arith::ConstantOp>(loc, type, llvm::cast<mlir::TypedAttr>(zeroAttr));
            lastValue = builder.create<mlir::arith::SubIOp>(loc, zero, val);
        }
    } else if (node.op == "!") {
        auto type = builder.getI1Type();
        auto oneAttr = builder.getIntegerAttr(type, 1);
        auto one = builder.create<mlir::arith::ConstantOp>(loc, type, llvm::cast<mlir::TypedAttr>(oneAttr));
        lastValue = builder.create<mlir::arith::XOrIOp>(loc, val, one);
    }
}

void MLIRGenVisitor::visit(NumberNode& node) {
    auto type = builder.getI32Type();
    auto attr = builder.getIntegerAttr(type, node.value);
    lastValue = builder.create<mlir::arith::ConstantOp>(builder.getUnknownLoc(), type, llvm::cast<mlir::TypedAttr>(attr));
}

void MLIRGenVisitor::visit(RealNode& node) {
    auto type = builder.getF64Type();
    auto attr = builder.getFloatAttr(type, node.value);
    lastValue = builder.create<mlir::arith::ConstantOp>(builder.getUnknownLoc(), type, llvm::cast<mlir::TypedAttr>(attr));
}

void MLIRGenVisitor::visit(BooleanNode& node) {
    auto type = builder.getI1Type();
    auto attr = builder.getIntegerAttr(type, node.value ? 1 : 0);
    lastValue = builder.create<mlir::arith::ConstantOp>(builder.getUnknownLoc(), type, llvm::cast<mlir::TypedAttr>(attr));
}

void MLIRGenVisitor::visit(CharNode& node) {
    auto type = builder.getI8Type();
    auto attr = builder.getIntegerAttr(type, node.value);
    lastValue = builder.create<mlir::arith::ConstantOp>(builder.getUnknownLoc(), type, llvm::cast<mlir::TypedAttr>(attr));
}

void MLIRGenVisitor::visit(PrintNode& node) {
    // 1. Calcola il valore dell'espressione
    node.expression->accept(*this);
    mlir::Value arg = lastValue;
    mlir::Type type = arg.getType();

    // 2. GESTIONE MEMORIA (MemRef)
    // Se 'arg' è un indirizzo di memoria, dobbiamo decidere se fare Load o no.
    if (auto memRefType = mlir::dyn_cast<mlir::MemRefType>(type)) {

        // A. È una STRINGA? (Array di char: elemento i8 e rank > 0)
        // Le stringhe NON si caricano con LoadOp, si passa il puntatore.
        if (memRefType.getElementType().isInteger(8) && memRefType.getRank() > 0) {

            // Dobbiamo fare un Cast a memref<?xi8> (dimensione dinamica)
            // perché la funzione runtime 'print_string' si aspetta quello.
            auto dynamicStringType = mlir::MemRefType::get(
                {mlir::ShapedType::kDynamic}, // Dimensione ?
                builder.getI8Type()           // Tipo i8
            );

            // memref.cast %arg : memref<10xi8> to memref<?xi8>
            arg = builder.create<mlir::memref::CastOp>(builder.getUnknownLoc(), dynamicStringType, arg);

            // Chiamiamo subito e usciamo
            builder.create<mlir::func::CallOp>(builder.getUnknownLoc(), "print_string", mlir::ValueRange{arg});
            return;
        }

        // B. È uno SCALARE? (Int, Double, Char singolo)
        // Se è un puntatore a un numero singolo, CARICHIAMO il valore.
        arg = builder.create<mlir::memref::LoadOp>(builder.getUnknownLoc(), arg);
        type = arg.getType(); // Aggiorniamo 'type': ora è il valore (es. i32), non più memref
    }

    // 3. SELEZIONE FUNZIONE (per scalari caricati)
    std::string funcName;

    if (type.isInteger(32)) {
        funcName = "print_int";
    }
    else if (type.isF64()) {
        funcName = "print_double";
    }
    else if (type.isInteger(8)) {
        funcName = "print_char"; // Char
    }
    else if (type.isInteger(1)) {
        // Bool: MLIR usa i1, ma C usa int. Estendiamo a 32 bit (Zero Extension).
        arg = builder.create<mlir::arith::ExtUIOp>(builder.getUnknownLoc(), builder.getI32Type(), arg);
        funcName = "print_int"; // Stampiamo 0 o 1
    }
    else {
        std::cerr << "Errore: Tipo non supportato per la print: "  << "\n";
        return;
    }

    // 4. Genera la chiamata
    builder.create<mlir::func::CallOp>(builder.getUnknownLoc(), funcName, mlir::ValueRange{arg});
}
void MLIRGenVisitor::visit(FunctionCallNode& node) {
    std::vector<mlir::Value> args;
    for(auto& argExpr : node.arguments) {
        argExpr->accept(*this);
        args.push_back(lastValue);
    }

    auto calle = theModule.lookupSymbol<mlir::func::FuncOp>(node.functionName);

    auto callOp = builder.create<mlir::func::CallOp>(
      builder.getUnknownLoc(), node.functionName, calle.getResultTypes() ,args);

    if (callOp.getNumResults() > 0) {
        lastValue = callOp.getResult(0);
    }
}

// Nodi non usati o vuoti
void MLIRGenVisitor::visit(ReadNode& node) {}
void MLIRGenVisitor::visit(StringNode& node) {}
void MLIRGenVisitor::visit(TypeNode& node) {}
void MLIRGenVisitor::visit(VoidNode& node) {}