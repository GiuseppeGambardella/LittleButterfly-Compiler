#include "mlir_gen_visitor.hpp"

// ==========================================================
// COSTRUTTORE E HELPER DI BASE
// ==========================================================

MLIRGenVisitor::MLIRGenVisitor(mlir::MLIRContext& ctx, SymbolTable& symTable)
    : context(ctx), symTable(symTable), builder(&ctx) {

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

    auto lhsType = lhs.getType();
    auto rhsType = rhs.getType();

    // --------------------------------------------------
    // 1. OPERATORI LOGICI: and / or
    // --------------------------------------------------
    if (node.op == "and" || node.op == "or") {

        // Normalizza LHS a i1
        if (!lhsType.isInteger(1)) {
            auto zero = builder.create<mlir::arith::ConstantIntOp>(loc, 0, 32);
            lhs = builder.create<mlir::arith::CmpIOp>(
                loc, mlir::arith::CmpIPredicate::ne, lhs, zero
            );
        }

        // Normalizza RHS a i1
        if (!rhsType.isInteger(1)) {
            auto zero = builder.create<mlir::arith::ConstantIntOp>(loc, 0, 32);
            rhs = builder.create<mlir::arith::CmpIOp>(
                loc, mlir::arith::CmpIPredicate::ne, rhs, zero
            );
        }

        // AND / OR
        if (node.op == "and") {
            lastValue = builder.create<mlir::arith::AndIOp>(loc, lhs, rhs);
        } else {
            lastValue = builder.create<mlir::arith::OrIOp>(loc, lhs, rhs);
        }

        return;
    }

    // --------------------------------------------------
    // 2. CONFRONTI
    // --------------------------------------------------
    if (node.op == "==" || node.op == "!=" ||
        node.op == "<"  || node.op == "<=" ||
        node.op == ">"  || node.op == ">=") {

        bool isFloat = lhsType.isF64();

        if (isFloat) {
            mlir::arith::CmpFPredicate pred;

            if (node.op == "==") pred = mlir::arith::CmpFPredicate::OEQ;
            else if (node.op == "!=") pred = mlir::arith::CmpFPredicate::ONE;
            else if (node.op == "<")  pred = mlir::arith::CmpFPredicate::OLT;
            else if (node.op == "<=") pred = mlir::arith::CmpFPredicate::OLE;
            else if (node.op == ">")  pred = mlir::arith::CmpFPredicate::OGT;
            else                      pred = mlir::arith::CmpFPredicate::OGE;

            lastValue = builder.create<mlir::arith::CmpFOp>(loc, pred, lhs, rhs);
        } else {
            mlir::arith::CmpIPredicate pred;

            if (node.op == "==") pred = mlir::arith::CmpIPredicate::eq;
            else if (node.op == "!=") pred = mlir::arith::CmpIPredicate::ne;
            else if (node.op == "<")  pred = mlir::arith::CmpIPredicate::slt;
            else if (node.op == "<=") pred = mlir::arith::CmpIPredicate::sle;
            else if (node.op == ">")  pred = mlir::arith::CmpIPredicate::sgt;
            else                      pred = mlir::arith::CmpIPredicate::sge;

            lastValue = builder.create<mlir::arith::CmpIOp>(loc, pred, lhs, rhs);
        }

        return;
    }

    // --------------------------------------------------
    // 3. ARITMETICA
    // --------------------------------------------------
    bool isFloat = lhsType.isF64();

    if (node.op == "+") {
        lastValue = isFloat
            ? builder.create<mlir::arith::AddFOp>(loc, lhs, rhs).getResult()
            : builder.create<mlir::arith::AddIOp>(loc, lhs, rhs).getResult();
    }
    else if (node.op == "-") {
        lastValue = isFloat
            ? builder.create<mlir::arith::SubFOp>(loc, lhs, rhs).getResult()
            : builder.create<mlir::arith::SubIOp>(loc, lhs, rhs).getResult();
    }
    else if (node.op == "*") {
        lastValue = isFloat
            ? builder.create<mlir::arith::MulFOp>(loc, lhs, rhs).getResult()
            : builder.create<mlir::arith::MulIOp>(loc, lhs, rhs).getResult();
    }
    else if (node.op == "/") {
        lastValue = isFloat
            ? builder.create<mlir::arith::DivFOp>(loc, lhs, rhs).getResult()
            : builder.create<mlir::arith::DivSIOp>(loc, lhs, rhs).getResult();
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

/*void MLIRGenVisitor::visit(PrintNode& node) {
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
}*/

void MLIRGenVisitor::visit(PrintNode& node) {
    // 1. Calcola il valore dell'espressione
    node.expression->accept(*this);
    mlir::Value arg = lastValue;
    mlir::Type type = arg.getType();
    auto loc = builder.getUnknownLoc();   // 🔧 [AGGIUNTO]

    // 2. GESTIONE MEMORIA (MemRef)
    if (auto memRefType = mlir::dyn_cast<mlir::MemRefType>(type)) {

        // A. STRINGA: passa il puntatore (NO load)
        if (memRefType.getElementType().isInteger(8) &&
            memRefType.getRank() > 0) {

            auto dynamicStringType = mlir::MemRefType::get(
                {mlir::ShapedType::kDynamic},
                builder.getI8Type()
            );

            arg = builder.create<mlir::memref::CastOp>(
                loc, dynamicStringType, arg   // 🔧 usa loc
            );

            // 🔧 FIX CRITICO: call con argomento
            builder.create<mlir::func::CallOp>(
                loc,
                "print_string",
                mlir::TypeRange{},            // 🔧 aggiunto
                mlir::ValueRange{arg}         // 🔧 già corretto
            );
            return;
        }

        // B. SCALARE → load
        arg = builder.create<mlir::memref::LoadOp>(loc, arg); // 🔧 usa loc
        type = arg.getType();
    }

    // 3. SELEZIONE FUNZIONE
    std::string funcName;

    if (type.isInteger(32)) {
        funcName = "print_int";
    }
    else if (type.isF64()) {
        funcName = "print_double";
    }
    else if (type.isInteger(8)) {
        funcName = "print_char";
    }
    else if (type.isInteger(1)) {
        arg = builder.create<mlir::arith::ExtUIOp>(
            loc, builder.getI32Type(), arg   // 🔧 usa loc
        );
        funcName = "print_int";
    }
    else {
        std::cerr << "Errore: Tipo non supportato per la print\n";
        return;
    }

    // 4. CALL CORRETTA (sempre con argomento)
    builder.create<mlir::func::CallOp>(
        loc,
        funcName,
        mlir::TypeRange{},                  // 🔧 FIX IMPORTANTE
        mlir::ValueRange{arg}
    );
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

void MLIRGenVisitor::visit(ReadNode& node) {
    // 1. read(x) → x deve essere una variabile
    auto* var = dynamic_cast<VariableNode*>(node.variable.get());
    if (!var) {
        std::cerr << "Errore: read() richiede una variabile\n";
        return;
    }

    // 2. Recupera info semantica dalla symbol table
    const auto& varInfo = symTable.lookup(var->name);
    BasicType varType = varInfo->type;

    // 3. Scegli la funzione runtime in base al tipo
    std::string fn;
    switch (varType) {
        case BasicType::INT:
            fn = "read_int";
            break;
        case BasicType::DOUBLE:
            fn = "read_double";
            break;
        case BasicType::CHAR:
            fn = "read_char";
            break;
        case BasicType::BOOL:
            // scelta: leggi int e poi converti a bool
            fn = "read_int";
            break;
        default:
            std::cerr << "Errore: tipo non supportato in read\n";
            return;
    }

    // 4. Recupera la funzione runtime
    auto callee = theModule.lookupSymbol<mlir::func::FuncOp>(fn);
    if (!callee) {
        std::cerr << "Errore: funzione runtime '" << fn << "' non trovata\n";
        return;
    }

    // 5. Call runtime: () -> T
    auto call = builder.create<mlir::func::CallOp>(
        builder.getUnknownLoc(),
        fn,
        callee.getResultTypes(),
        mlir::ValueRange{}
    );

    mlir::Value value = call.getResult(0);

    // 6. Se BOOL: normalizza a i1 (value != 0)
    if (varType == BasicType::BOOL) {
        auto zero = builder.create<mlir::arith::ConstantIntOp>(
            builder.getUnknownLoc(), 0, 32
        );

        value = builder.create<mlir::arith::CmpIOp>(
            builder.getUnknownLoc(),
            mlir::arith::CmpIPredicate::ne,
            value,
            zero
        ); // i1
    }

    // 7. Store nella variabile (nel tuo modello: globale)
    mlir::Value addr = getGlobalAddress(var->name);
    if (!addr) {
        std::cerr << "Errore: variabile '" << var->name << "' non trovata\n";
        return;
    }

    builder.create<mlir::memref::StoreOp>(
        builder.getUnknownLoc(),
        value,
        addr
    );
}


void MLIRGenVisitor::visit(StringNode& node) {
    std::string globalName;

    // 1. String pooling: riusa globale se già esistente
    auto it = stringPool.find(node.value);
    if (it != stringPool.end()) {
        globalName = it->second;
    } else {
        globalName = ".str" + std::to_string(stringLiteralCounter++);
        stringPool[node.value] = globalName;

        // 2. Costruisci stringa con terminatore '\0'
        std::string s = node.value;
        s.push_back('\0');

        const int64_t len = static_cast<int64_t>(s.size());
        auto i8 = builder.getI8Type();
        auto memrefType = mlir::MemRefType::get({len}, i8);

        // 3. Inizializzatore: DenseElementsAttr
        llvm::SmallVector<int8_t, 32> bytes;
        for (unsigned char c : s)
            bytes.push_back(static_cast<int8_t>(c));

        auto tensorType = mlir::RankedTensorType::get({len}, i8);
        auto initAttr = mlir::DenseElementsAttr::get(
            tensorType,
            llvm::ArrayRef<int8_t>(bytes)
        );

        // 4. Crea memref.global nel modulo
        {
            mlir::OpBuilder::InsertionGuard guard(builder);
            builder.setInsertionPointToEnd(theModule.getBody());

            builder.create<mlir::memref::GlobalOp>(
                builder.getUnknownLoc(),
                globalName,
                builder.getStringAttr("private"),
                memrefType,
                initAttr,
                /*constant=*/true,
                /*alignment=*/nullptr
            );
        }
    }

    // 5. get_global → valore della stringa
    auto global = theModule.lookupSymbol<mlir::memref::GlobalOp>(globalName);
    lastValue = builder.create<mlir::memref::GetGlobalOp>(
        builder.getUnknownLoc(),
        global.getType(),
        globalName
    );
}

// Nodi non usati o vuoti
void MLIRGenVisitor::visit(TypeNode& node) {}
void MLIRGenVisitor::visit(VoidNode& node) {}