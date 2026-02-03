#include "mlir_gen_visitor.hpp"
#include <iostream>

// ==========================================================
// COSTRUTTORE E INIZIALIZZAZIONE
// ==========================================================

MLIRGenVisitor::MLIRGenVisitor(mlir::MLIRContext& ctx, SymbolTable& table)
    : context(ctx),
      symTable(table),
      builder(&ctx),
      // 1. Creiamo il modulo principale: è il contenitore radice di tutto il codice generato.
      theModule(mlir::ModuleOp::create(builder.getUnknownLoc())),
      // 2. Inizializziamo la Symbol Table di MLIR legandola al modulo.
      // Questa tabella gestirà la visibilità di funzioni e variabili globali nel codice intermedio.
      mlirSymTable(theModule)
{
    // Posizioniamo il cursore di inserimento (builder) alla fine del corpo del modulo.
    builder.setInsertionPointToEnd(theModule.getBody());

    // 3. Dichiariamo le funzioni di runtime (es. print, read) che il linguaggio può chiamare.
    declareRuntimeFunctions();

    // 4. Trasformiamo le variabili globali presenti nella tua SymbolTable (AST) in variabili MLIR.
    initializeGlobalsFromSymbolTable();
}

// Funzione di utilità per stampare a video il codice MLIR generato.
void MLIRGenVisitor::dump() {
    theModule.print(llvm::outs());
}

// Dichiarazione delle funzioni esterne (Runtime) scritte in C++ che verranno linkate.
void MLIRGenVisitor::declareRuntimeFunctions() {
    auto loc = builder.getUnknownLoc();
    // Definiamo i tipi primitivi di MLIR che useremo nelle firme delle funzioni.
    auto i32 = builder.getI32Type();
    auto f64 = builder.getF64Type();
    auto i8 = builder.getI8Type();
    auto i1 = builder.getI1Type(); // i1 è il tipo booleano (1 bit).
    // MemRef dinamico di i8 rappresenta una stringa o array di char.
    auto stringType = mlir::MemRefType::get({mlir::ShapedType::kDynamic}, i8);

    // Funzione helper (lambda) per creare la dichiarazione di una funzione e inserirla nella tabella MLIR.
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

// Helper per convertire i tipi del tuo linguaggio (BasicType) nei tipi di MLIR.
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
// CORE: POPOLAMENTO E LOOKUP TRAMITE MLIR::SYMBOLTABLE
// ==========================================================

void MLIRGenVisitor::initializeGlobalsFromSymbolTable() {
    // Iteriamo su tutti i simboli trovati durante l'analisi semantica.
    for (const auto& pair : symTable.getTable()) {
        const std::string& name = pair.first;
        const SymbolInfo& info = pair.second;

        // Gestiamo solo le variabili globali qui (le funzioni sono gestite in visit(ProgramNode)).
        if (!info.isFunction) {

            // Se esiste già nella tabella MLIR, saltiamo per evitare duplicati.
            if (mlirSymTable.lookup(name)) continue;


            if (info.type == BasicType::STRING) {
                continue; // le stringhe globali sono gestite come literal nel codice
            }
            mlir::Type type = getMLIRType(info.type);
            // Creiamo un MemRef (Memory Reference) scalare (rank vuoto {}) per la variabile.
            auto memRefType = mlir::MemRefType::get({}, type);

            // 1. Creiamo l'operazione 'memref.global'.
            // Questa istruzione alloca memoria statica per la variabile globale.
            auto globalOp = builder.create<mlir::memref::GlobalOp>(
                builder.getUnknownLoc(),
                name,
                builder.getStringAttr("private"),
                memRefType,
                mlir::Attribute(), // Valore iniziale nullo (non inizializzato qui)
                false,
                nullptr
            );


            globalOp->remove();
            // 2. INSERIRE ESPLICITAMENTE NELLA MLIR SYMBOL TABLE
            mlirSymTable.insert(globalOp);
        }
    }
}

// Helper per recuperare il valore (indirizzo di memoria) di una variabile globale dato il nome.
mlir::Value MLIRGenVisitor::getGlobalAddress(const std::string& name) {
    // 1. Cerchiamo l'operazione 'memref.global' nella tabella MLIR.
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

    // 2. Creiamo l'istruzione 'memref.get_global'.
    // Questa istruzione restituisce il puntatore alla memoria allocata, necessario per load/store.
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
    // È fondamentale dichiarare le funzioni prima di definirne il corpo per supportare
    // chiamate a funzioni definite successivamente nel file.

    // A. Funzioni definite dall'utente (presenti nelle dichiarazioni globali)
    for (auto& decl : node.globals) {
        if (auto funcDecl = dynamic_cast<FunctionDeclNode*>(decl.get())) {

            // Costruiamo la lista dei tipi degli argomenti
            llvm::SmallVector<mlir::Type, 4> argTypes;
            for (auto& param : funcDecl->parameters) {
                if (auto varDecl = dynamic_cast<VarDeclNode*>(param.get())) {
                    argTypes.push_back(getMLIRType(varDecl->type.type));
                }
            }

            // Costruiamo la lista dei tipi di ritorno (0 o 1 elemento)
            llvm::SmallVector<mlir::Type, 1> resultTypes;
            if (auto retNode = dynamic_cast<TypeNode*>(funcDecl->returnType.get())) {
                if (retNode->type != BasicType::VOID) {
                    resultTypes.push_back(getMLIRType(retNode->type));
                }
            }

            // Creiamo la funzione (FuncOp) e la inseriamo nella tabella
            auto funcType = builder.getFunctionType(argTypes, resultTypes);
            auto func = builder.create<mlir::func::FuncOp>(builder.getUnknownLoc(), funcDecl->name, funcType);

            func->remove();
            mlirSymTable.insert(func);
        }
    }

    // B. Funzione FLY (EntryPoint principale)
    if (node.mainBlock) {
        // Fly è trattata come una FunctionDeclNode speciale
        if (auto flyDecl = dynamic_cast<FunctionDeclNode*>(node.mainBlock.get())) {

            // Fly solitamente non ha argomenti
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
            // Creiamo la funzione con nome "fly"
            auto func = builder.create<mlir::func::FuncOp>(builder.getUnknownLoc(), flyDecl->name, funcType);

            func->remove();
            mlirSymTable.insert(func);
        }
    }

    // -------------------------------------------------------------
    // PASSO 2: Genera i corpi delle funzioni
    // -------------------------------------------------------------
    // Ora che i prototipi esistono, possiamo generare il codice interno.
    for (auto& decl : node.globals) {
        decl->accept(*this);
    }

    if (node.mainBlock) node.mainBlock->accept(*this);
}

void MLIRGenVisitor::visit(FunctionDeclNode& node) {
    // 1. Recupera la funzione dalla tabella (creata nel Passo 1)
    auto op = mlirSymTable.lookup(node.name);
    auto func = llvm::dyn_cast_or_null<mlir::func::FuncOp>(op);

    if (!func) {
        std::cerr << "ERRORE INTERNO: Funzione '" << node.name << "' persa.\n";
        return;
    }

    // Raccogliamo i nomi degli argomenti per associarli ai valori
    std::vector<std::string> argNames;
    for (auto& param : node.parameters) {
        if (auto varDecl = dynamic_cast<VarDeclNode*>(param.get())) {
            argNames.push_back(varDecl->name);
        }
    }

    // Se la funzione ha già dei blocchi, è già stata generata (evita rigenerazione)
    if (!func.getBlocks().empty()) return;

    // Crea il blocco d'ingresso (Entry Block) dove inizia l'esecuzione della funzione
    mlir::Block* entryBlock = func.addEntryBlock();
    builder.setInsertionPointToStart(entryBlock);

    // Gestione Argomenti: copia i valori passati come argomenti in variabili locali (Store)
    // così possono essere modificati all'interno della funzione.
    for (size_t i = 0; i < argNames.size(); ++i) {
        mlir::Value argValue = entryBlock->getArgument(i);
        const auto* info = symTable.lookup(argNames[i]);
        if (!info) continue;

        if (info->type == BasicType::STRING) {
            // binding del parametro stringa
            stringEnv[argNames[i]] = argValue;
            continue;
        }

        // solo per scalari
        mlir::Value globalPtr = getGlobalAddress(argNames[i]);
        if (!globalPtr) continue;

        builder.create<mlir::memref::StoreOp>(
            builder.getUnknownLoc(), argValue, globalPtr
        );

    }


    // Genera il codice del corpo della funzione
    if (node.body) node.body->accept(*this);

    // Controllo terminatore: verifica se l'Entry Block ha un'istruzione di ritorno.
    // NOTA: Controlla solo 'entryBlock', non il blocco corrente del builder.
    mlir::Block* currentBlock = builder.getBlock();
    bool hasTerminator = !currentBlock->empty() && currentBlock->back().hasTrait<mlir::OpTrait::IsTerminator>();

    if (!hasTerminator) {
        // Se manca il return, ne aggiungiamo uno implicito.
        auto resultTypes = func.getFunctionType().getResults();
        if (!resultTypes.empty()) {
             // Se la funzione deve ritornare un valore, ritorniamo 0 (o 0.0)
             auto t = resultTypes[0];
             mlir::Attribute z;
             if (t.isF64()) z = builder.getFloatAttr(t, 0.0);
             else z = builder.getIntegerAttr(t, 0);

             auto c = builder.create<mlir::arith::ConstantOp>(builder.getUnknownLoc(), t, llvm::cast<mlir::TypedAttr>(z));
             builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc(), mlir::ValueRange{c});
        } else {
             // Se è void, ritorniamo void
             builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc());
        }
    }
}

// ==========================================================
// ALTRI VISITOR (Istruzioni Standard)
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
    // Uso di una variabile in una espressione: Legge (Load) il valore dalla memoria

    const auto info = symTable.lookup(node.name);
    if (info->type == BasicType::STRING) {
        auto it = stringEnv.find(node.name);
        if (it == stringEnv.end()) {
            std::cerr << "ERRORE: stringa '" << node.name << "' non inizializzata\n";
        } else {
            lastValue = it->second;
        }
        return;
    }

    mlir::Value address = getGlobalAddress(node.name);

    if (address) {
        lastValue = builder.create<mlir::memref::LoadOp>(builder.getUnknownLoc(), address);
    } else {
        // Fallback: costante 0 se indirizzo non trovato
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
    // Visita tutte le istruzioni contenute nel blocco
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
    // Gestione IF tramite dialetto 'cf' (Control Flow - salti non strutturati)

    // 1. Valuta la condizione
    node.condition->accept(*this);
    mlir::Value cond = lastValue;

    // Recupera contesto corrente
    mlir::Block* currentBlock = builder.getBlock();
    mlir::Region* region = currentBlock->getParent();

    // 2. Crea i blocchi per i rami
    mlir::Block* thenBlock = builder.createBlock(region);
    mlir::Block* elseBlock = nullptr;
    if (node.elseBranch) {
        elseBlock = builder.createBlock(region);
    }
    mlir::Block* mergeBlock = builder.createBlock(region); // Blocco di uscita

    // 3. Genera salto condizionale (CondBranch)
    builder.setInsertionPointToEnd(currentBlock);
    if (elseBlock) {
        builder.create<mlir::cf::CondBranchOp>(builder.getUnknownLoc(), cond, thenBlock, elseBlock);
    } else {
        builder.create<mlir::cf::CondBranchOp>(builder.getUnknownLoc(), cond, thenBlock, mergeBlock);
    }

    // 4. Genera codice Ramo THEN
    builder.setInsertionPointToStart(thenBlock);
    node.thenBranch->accept(*this);
    // Salta al merge se il blocco non è terminato (es. da un return)
    if (builder.getBlock()->empty() || !builder.getBlock()->back().hasTrait<mlir::OpTrait::IsTerminator>()) {
        builder.create<mlir::cf::BranchOp>(builder.getUnknownLoc(), mergeBlock);
    }

    // 5. Genera codice Ramo ELSE (se presente)
    if (elseBlock) {
        builder.setInsertionPointToStart(elseBlock);
        node.elseBranch->accept(*this);
        // Salta al merge se il blocco non è terminato
        if (builder.getBlock()->empty() || !builder.getBlock()->back().hasTrait<mlir::OpTrait::IsTerminator>()) {
            builder.create<mlir::cf::BranchOp>(builder.getUnknownLoc(), mergeBlock);
        }
    }

    // 6. Continua generazione dal blocco Merge
    builder.setInsertionPointToStart(mergeBlock);
}

void MLIRGenVisitor::visit(LoopNode& node) {
    // Recupera il blocco corrente e la regione
    mlir::Block* currentBlock = builder.getBlock();
    mlir::Region* region = currentBlock->getParent();

    // 1. Crea i blocchi per il ciclo: Header (condizione), Body (corpo), Exit (uscita)
    mlir::Block* headerBlock = builder.createBlock(region);
    mlir::Block* bodyBlock = builder.createBlock(region);
    mlir::Block* exitBlock = builder.createBlock(region);

    // 2. Salta dal blocco corrente all'Header (inizio del ciclo)
    builder.setInsertionPointToEnd(currentBlock);
    builder.create<mlir::cf::BranchOp>(builder.getUnknownLoc(), headerBlock);

    // --- HEADER: Valutazione Condizione ---
    builder.setInsertionPointToStart(headerBlock);
    node.condition->accept(*this);
    // Se la condizione è vera vai al Body, se falsa vai all'Exit
    builder.create<mlir::cf::CondBranchOp>(builder.getUnknownLoc(), lastValue, bodyBlock, exitBlock);

    // --- BODY: Esecuzione Istruzioni ---
    builder.setInsertionPointToStart(bodyBlock);
    if (node.body) node.body->accept(*this);

    // 3. Back-edge: Salta indietro all'Header per ripetere il ciclo
    // IMPORTANTE: Aggiungi il salto solo se il blocco non è già terminato (es. da un return)
    mlir::Block* bodyEndBlock = builder.getBlock();
    if (bodyEndBlock->empty() || !bodyEndBlock->back().hasTrait<mlir::OpTrait::IsTerminator>()) {
        builder.create<mlir::cf::BranchOp>(builder.getUnknownLoc(), headerBlock);
    }

    // 4. Posiziona il builder sull'Exit per continuare con il codice successivo
    builder.setInsertionPointToStart(exitBlock);
}

void MLIRGenVisitor::visit(BinaryOpNode& node) {
    // Visita operandi
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
            std::cerr << "ERRORE: strcmp_strings non trovata\n";
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

    // Gestione operatori logici (AND / OR)
    // Converte eventuali i32 (0/1) in i1 (bool) per le operazioni logiche
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

    // Gestione Concatenazione Stringhe (&)
    if (node.op == "&") {
        // Lambda per convertire qualsiasi tipo in stringa usando le funzioni runtime
        auto convertToString = [&](mlir::Value v, mlir::Type t) -> mlir::Value {
            std::string fnName;
            if (t.isInteger(32)) fnName = "to_string_int";
            else if (t.isInteger(1)) fnName = "to_string_bool";
            else if (t.isF64()) fnName = "to_string_double";
            else if (t.isInteger(8)) fnName = "to_string_char";
            else if (auto memTy = llvm::dyn_cast<mlir::MemRefType>(t)) {
                if (memTy.getElementType().isInteger(8)) {
                    // Cast a stringa dinamica se necessario
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

        // Chiama concat_strings
        if (lhsStr && rhsStr) {
            auto op = mlirSymTable.lookup("concat_strings");
            auto concatFn = llvm::dyn_cast<mlir::func::FuncOp>(op);
            if (concatFn) {
                lastValue = builder.create<mlir::func::CallOp>(loc, concatFn, mlir::ValueRange{lhsStr, rhsStr}).getResult(0);
            }
        }
        return;
    }

    // Gestione Operatori Aritmetici (+, -, *, /) e Confronti (==, <, ecc.)
    // Distingue tra operazioni Floating Point (F) e Intere (I/S)
    bool isFloat = lhsType.isF64();
    if(node.op=="+") lastValue = isFloat ? builder.create<mlir::arith::AddFOp>(loc,lhs,rhs).getResult() : builder.create<mlir::arith::AddIOp>(loc,lhs,rhs).getResult();
    else if(node.op=="-") lastValue = isFloat ? builder.create<mlir::arith::SubFOp>(loc,lhs,rhs).getResult() : builder.create<mlir::arith::SubIOp>(loc,lhs,rhs).getResult();
    else if(node.op=="*") lastValue = isFloat ? builder.create<mlir::arith::MulFOp>(loc,lhs,rhs).getResult() : builder.create<mlir::arith::MulIOp>(loc,lhs,rhs).getResult();
    else if(node.op=="/") lastValue = isFloat ? builder.create<mlir::arith::DivFOp>(loc,lhs,rhs).getResult() : builder.create<mlir::arith::DivSIOp>(loc,lhs,rhs).getResult();
    // Confronti
    else if (node.op == "==") {

        if(isFloat) lastValue = builder.create<mlir::arith::CmpFOp>(loc, mlir::arith::CmpFPredicate::OEQ, lhs, rhs);
        else lastValue = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::eq, lhs, rhs);
    }
    else if (node.op == "<>") {
        if(isFloat) lastValue = builder.create<mlir::arith::CmpFOp>(loc, mlir::arith::CmpFPredicate::ONE, lhs, rhs);
        else lastValue = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::ne, lhs, rhs);
    }
    // ... altri operatori di confronto ...
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
    // Meno unario: sottrazione da zero (0 - x)
    if (node.op == "-") {
        if (val.getType().isF64()) {
            auto zero = builder.create<mlir::arith::ConstantOp>(loc, builder.getF64Type(), builder.getFloatAttr(builder.getF64Type(), 0.0));
            lastValue = builder.create<mlir::arith::SubFOp>(loc, zero, val);
        } else {
            auto zero = builder.create<mlir::arith::ConstantOp>(loc, val.getType(), builder.getIntegerAttr(val.getType(), 0));
            lastValue = builder.create<mlir::arith::SubIOp>(loc, zero, val);
        }
    } else if (node.op == "!") {
        // Not logico: XOR con 1
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

void MLIRGenVisitor::visit(NumberNode& node) { lastValue = builder.create<mlir::arith::ConstantIntOp>(builder.getUnknownLoc(), node.value, 32); }
void MLIRGenVisitor::visit(RealNode& node) { lastValue = builder.create<mlir::arith::ConstantOp>(builder.getUnknownLoc(), builder.getF64Type(), builder.getFloatAttr(builder.getF64Type(), node.value)); }
void MLIRGenVisitor::visit(BooleanNode& node) { lastValue = builder.create<mlir::arith::ConstantIntOp>(builder.getUnknownLoc(), node.value, 1); }
void MLIRGenVisitor::visit(CharNode& node) { lastValue = builder.create<mlir::arith::ConstantIntOp>(builder.getUnknownLoc(), node.value, 8); }

void MLIRGenVisitor::visit(ReadNode& node) {
    auto* var = dynamic_cast<VariableNode*>(node.variable.get());
    if(!var) return;
    const auto* info = symTable.lookup(var->name);

    // Determina la funzione di lettura da chiamare in base al tipo
    std::string fn = "read_int";
    if(info->type == BasicType::DOUBLE) fn = "read_double";
    else if(info->type == BasicType::CHAR) fn = "read_char";
    else if (info->type == BasicType::STRING) fn = "read_string";

    auto op = mlirSymTable.lookup(fn);
    auto callee = llvm::dyn_cast<mlir::func::FuncOp>(op);
    auto call = builder.create<mlir::func::CallOp>(builder.getUnknownLoc(), callee, mlir::ValueRange{});
    mlir::Value val = call.getResult(0);

    // Se bool, converti risultato lettura (int) in bool (ne 0)
    if (info->type == BasicType::BOOL) {
         auto zero = builder.create<mlir::arith::ConstantIntOp>(builder.getUnknownLoc(), 0, 32);
         val = builder.create<mlir::arith::CmpIOp>(builder.getUnknownLoc(), mlir::arith::CmpIPredicate::ne, val, zero);
    }

    if (info->type == BasicType::STRING) {
        stringEnv[var->name] = val; // <-- salva la stringa letta
        return;
    }

    // Salva il valore letto nella variabile
    mlir::Value addr = getGlobalAddress(var->name);
    if (!addr) return;

    builder.create<mlir::memref::StoreOp>(
        builder.getUnknownLoc(), val, addr
    );

}

void MLIRGenVisitor::visit(FunctionCallNode& node) {
    // Risolve argomenti
    std::vector<mlir::Value> args;
    for(auto& a : node.arguments) { a->accept(*this); args.push_back(lastValue); }

    // Esegue la chiamata (CallOp)
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

    // Gestione stampa stringhe (MemRef)
    if (auto memRefType = mlir::dyn_cast<mlir::MemRefType>(type)) {
        if (memRefType.getElementType().isInteger(8) && memRefType.getRank() > 0) {
            // Cast a tipo dinamico e chiama print_string
            auto dynamicStringType = mlir::MemRefType::get({mlir::ShapedType::kDynamic}, builder.getI8Type());
            arg = builder.create<mlir::memref::CastOp>(loc, dynamicStringType, arg);
            builder.create<mlir::func::CallOp>(loc, "print_string", mlir::TypeRange{}, mlir::ValueRange{arg});
            return;
        }
        // Se non è stringa, è puntatore a scalare -> Load
        arg = builder.create<mlir::memref::LoadOp>(loc, arg);
        type = arg.getType();
    }

    // Seleziona funzione di stampa in base al tipo
    std::string funcName = "print_int";
    if (type.isF64()) funcName = "print_double";
    else if (type.isInteger(8)) funcName = "print_char";
    else if (type.isInteger(1)) {
        arg = builder.create<mlir::arith::ExtUIOp>(loc, builder.getI32Type(), arg);
    }
    builder.create<mlir::func::CallOp>(loc, funcName, mlir::TypeRange{}, mlir::ValueRange{arg});
}

void MLIRGenVisitor::visit(StringNode& node) {
    // Gestione String Literals ("testo")
    std::string globalName;

    // Se la stringa esiste già nel pool, la riusiamo
    if (stringPool.find(node.value) != stringPool.end()) {
        globalName = stringPool[node.value];
    } else {
        // Altrimenti creiamo una nuova costante globale privata
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

    // Cast "alla radice": trasformiamo subito la stringa statica in dinamica (memref<?xi8>)
    auto dynamicStrType = mlir::MemRefType::get({mlir::ShapedType::kDynamic}, builder.getI8Type());
    lastValue = builder.create<mlir::memref::CastOp>(builder.getUnknownLoc(), dynamicStrType, staticPtr);
}

void MLIRGenVisitor::visit(TypeNode&) {}
void MLIRGenVisitor::visit(VoidNode&) {}

void MLIRGenVisitor::emitMainWrapper() {
    auto loc = builder.getUnknownLoc();
    // Crea la funzione "main" standard C che chiama "fly"
    auto mainFunc = builder.create<mlir::func::FuncOp>(loc, "main", builder.getFunctionType({}, builder.getI32Type()));
    mainFunc->remove();
    mlirSymTable.insert(mainFunc);

    builder.setInsertionPointToStart(mainFunc.addEntryBlock());

    // Chiama fly
    auto op = mlirSymTable.lookup("fly");
    if (auto fly = llvm::dyn_cast_or_null<mlir::func::FuncOp>(op)) {
        builder.create<mlir::func::CallOp>(loc, fly, mlir::ValueRange{});
    }

    // Ritorna 0
    auto zero = builder.create<mlir::arith::ConstantIntOp>(loc, 0, 32);
    builder.create<mlir::func::ReturnOp>(loc, mlir::ValueRange{zero});
}