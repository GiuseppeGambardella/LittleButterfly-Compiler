#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

// --- CONFIGURAZIONE PERCORSI ---
const std::string POSITIVE_DIR = "../testing/syntax/valid";
const std::string NEGATIVE_DIR_1 = "../testing/syntax/invalid";
const std::string NEGATIVE_DIR_2 = "../testing/semantic/invalid";

#ifdef _WIN32
const std::string COMPILER_EXE = "LittleButterfly_Compiler.exe";
#else
const std::string COMPILER_EXE = "./LittleButterfly_Compiler";
#endif

// Helper per eseguire una categoria di test
void run_category(const std::string& dirPath, bool expectSuccess, int& passedCount, int& totalCount) {
    if (!fs::exists(dirPath)) {
        std::cout << "[SKIP] Cartella non trovata: " << dirPath << std::endl;
        return;
    }

    std::cout << "--- Esecuzione test in: " << fs::path(dirPath).filename().string()
              << " (Atteso: " << (expectSuccess ? "SUCCESS" : "FAILURE") << ") ---" << std::endl;

    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (entry.path().extension() == ".lb") {
            totalCount++;
            std::string filename = entry.path().filename().string();
            std::string fullPath = entry.path().string();

            std::cout << "Test: " << filename;
            int padding = 35 - filename.length();
            if (padding > 0) std::cout << std::string(padding, ' ');

            // Esegui nascondendo l'output
            std::string cmd = "\"" + COMPILER_EXE + "\" " + fullPath + " > nul 2>&1";
            int retCode = std::system(cmd.c_str());

            bool isSuccess = (retCode == 0);

            // LOGICA DI VERIFICA
            if (expectSuccess) {
                // TEST POSITIVI: Devono dare 0
                if (isSuccess) {
                    std::cout << " -> [PASS]" << std::endl;
                    passedCount++;
                } else {
                    std::cout << " -> [FAIL] (Errore inaspettato)" << std::endl;
                }
            } else {
                // TEST NEGATIVI: Devono dare ERRORE (!= 0)
                if (!isSuccess) {
                    std::cout << " -> [PASS] (Errore rilevato correttamente)" << std::endl;
                    passedCount++;
                } else {
                    std::cout << " -> [FAIL] (Ha compilato codice errato!)" << std::endl;
                }
            }
        }
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "   LITTLE BUTTERFLY COMPLETE TEST SUITE    " << std::endl;
    std::cout << "===========================================" << std::endl << std::endl;

    if (!fs::exists(COMPILER_EXE)) {
        std::cerr << "[ERRORE] Compilatore non trovato: " << COMPILER_EXE << std::endl;
        std::cin.get();
        return 1;
    }

    int passed = 0;
    int total = 0;

    // 1. Esegui i test positivi (Codice valido)
    run_category(POSITIVE_DIR, true, passed, total);

    // 2. Esegui i test negativi (Codice errato)
    run_category(NEGATIVE_DIR_1, false, passed, total);

    run_category(NEGATIVE_DIR_2, false, passed, total);

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "RIEPILOGO: " << passed << "/" << total << " test superati." << std::endl;

    if (total > 0 && passed == total) {
        std::cout << "\n[OTTIMO] Il compilatore accetta il giusto e rifiuta lo sbagliato." << std::endl;
    } else {
        std::cout << "\n[ATTENZIONE] Qualcosa non va. Controlla i [FAIL]." << std::endl;
    }

    // std::cin.get();
    return 0;
}