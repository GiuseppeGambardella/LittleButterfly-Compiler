<div align="center">

# 🦋 LittleButterfly Compiler

**Un compilatore sperimentale basato su LLVM e MLIR**

![C++](https://img.shields.io/badge/Standard-C%2B%2B20-blue?logo=c%2B%2B)
![Build](https://img.shields.io/badge/Build-CMake-orange?logo=cmake)
![LLVM](https://img.shields.io/badge/Backend-LLVM%20%2F%20MLIR-green?logo=llvm)

</div>

---

**LittleButterfly** è un linguaggio di programmazione custom che dimostra l'implementazione di una pipeline di compilazione moderna. Utilizza **Flex** e **Bison** per il frontend e sfrutta la potenza di **MLIR** (Multi-Level Intermediate Representation) per generare codice macchina ottimizzato tramite **LLVM**.

## 📑 Indice

- [⚡ Caratteristiche del Linguaggio](#-caratteristiche-del-linguaggio)
- [📂 Struttura del Progetto](#-struttura-del-progetto)
- [🛠 Prerequisiti e Dipendenze](#-prerequisiti-e-dipendenze)
- [🏗 Build e Installazione](#-build-e-installazione)
- [🚀 Esecuzione](#-esecuzione)
- [📝 Esempi di Codice](#-esempi-di-codice)

---

## ⚡ Caratteristiche del Linguaggio

LittleButterfly utilizza una sintassi unica e verbosa per le sue keyword. Ecco la guida di riferimento rapida:

### 🧱 Tipi di Dato
| Keyword LittleButterfly | Corrispettivo C++ | Descrizione |
| :--- | :--- | :--- |
| `ifint` | `int` | Numero Intero |
| `refeafal` | `double` | Numero Reale (Float) |
| `bofoofol` | `bool` | Valore Booleano |
| `chafar` | `char` | Singolo Carattere |
| `strifing` | `std::string` | Stringa di testo |
| `notype` | `void` | Tipo vuoto/nullo |

### 🎛 Controllo di Flusso
| Struttura | Sintassi |
| :--- | :--- |
| **Condizione** | `ifif (cond) thefen { ... } efelsefe { ... }` |
| **Ciclo** | `whifilefe (cond) thefen { ... }` |
| **Funzione** | `defef nome(params) : tipo { ... }` |
| **Ritorno** | `flyback;` |
| **Main** | `fly() { ... }` |

### 📺 Input / Output
* **Stampa:** `flyout(valore)`
* **Input:** `flyin(variabile)`

---

## 📂 Struttura del Progetto

```text
LittleButterfly-Compiler/
├── 📁 ast/             # Definizioni dei nodi dell'Abstract Syntax Tree
├── 📁 lexer/           # Analizzatore lessicale (Flex: lexer.l)
├── 📁 parser/          # Analizzatore sintattico (Bison: parser.y)
├── 📁 semantic/        # Symbol Table e Type Checking
├── 📁 mlir/            # Generazione MLIR e Lowering verso LLVM IR
├── 📁 runtime/         # Libreria C++ di supporto a runtime
├── 📁 testing/         # Suite di test automatizzati e file .lb
├── 📄 CMakeLists.txt   # Configurazione di build principale
└── 📄 main.cpp         # Entry point del compilatore
```

---

## 🛠 Prerequisiti e Dipendenze

Per compilare il progetto è necessario un ambiente di sviluppo C++ moderno.

### Requisiti Base
* **Compilatore C++**: Compatibile con **C++20**.
* **CMake**: Versione **3.14+**.
* **LLVM & MLIR**: Installazione completa con librerie di sviluppo.

### Installazione Flex & Bison

Il progetto richiede Flex e Bison per generare il parser e il lexer. Ecco come installarli sulla tua piattaforma:

#### 🐧 Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install flex bison
```

#### 🍎 macOS
Si consiglia l'uso di Homebrew:
```bash
brew install flex bison
```
*Nota: CMake cercherà automaticamente in `/opt/homebrew/opt/bison/bin`.*

#### 🪟 Windows
1.  **Via WSL2 (Consigliato):** Installa Ubuntu su WSL e segui le istruzioni Linux. È il metodo più stabile per LLVM.
2.  **Nativo:** Installa **WinFlexBison** tramite Chocolatey:
    ```powershell
    choco install winflexbison3
    ```

---

## 🏗 Build e Installazione

Segui questi passaggi per compilare il progetto da zero:

1.  **Clona il repository**
    ```bash
    git clone [https://github.com/giuseppegambardella/LittleButterfly-Compiler.git](https://github.com/giuseppegambardella/LittleButterfly-Compiler.git)
    cd LittleButterfly-Compiler
    ```

2.  **Crea la directory di build**
    ```bash
    mkdir build && cd build
    ```

3.  **Configura con CMake**

    *Opzione A: Installazione Standard (Linux/macOS)*
    ```bash
    cmake ..
    ```

    *Opzione B: Percorso Custom (Consigliato per Windows/Dev)*
    Se hai compilato LLVM da sorgente o l'hai installato in una cartella specifica, usa `CMAKE_PREFIX_PATH` per indicare la cartella di build/installazione di LLVM:
    ```bash
    cmake -DCMAKE_PREFIX_PATH="C:/.../llvm-project/build" ..
    ```
    *(Sostituisci il percorso con quello della tua installazione)*

4.  **Compila**
    ```bash
    make
    # Oppure su Windows: cmake --build . --config Release
    ```
---

## 🚀 Esecuzione

### Compilazione di un sorgente
Una volta compilato, l'eseguibile `LittleButterfly_Compiler` si troverà nella cartella `build`.

```bash
./LittleButterfly_Compiler ../testing/tests_suite/1.lb
```

### Esecuzione della Test Suite
Il progetto include un `TestRunner` integrato per verificare le funzionalità:

```bash
./TestRunner
```

---

## 📝 Esempi di Codice

Ecco un esempio completo di un programma `.lb` che calcola una somma semplice con logica condizionale:

```c++
@* Esempio: Calcolo Semplice *@

fly() {
    ifint a = 10;
    ifint b = 20;
    
    @* Controllo condizionale *@
    ifif a < b thefen {
        flyout("A è minore di B");
    } efelsefe {
        flyout("A è maggiore o uguale a B");
    }
    
    ifint somma = a + b;
    
    @* Output del risultato *@
    flyout(somma);
    
    flyback;
}
```

<div align="center">
  <sub>LittleButterfly Compiler Project</sub>
</div>
