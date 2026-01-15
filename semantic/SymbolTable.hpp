#pragma once
#include <string>
#include <unordered_map>
#include <iostream>
#include "../ast/nodes/expression/TypeNode.hpp"
#include <vector>
#include "helper/helper.h"

// Informazioni associate a ogni simbolo
struct SymbolInfo {
    BasicType type;
    bool isFunction;
    std::vector<BasicType> paramTypes; // Tipi dei parametri se è una funzione
};

class SymbolTable {
private:
    std::unordered_map<std::string, SymbolInfo> table;

public:

    const std::unordered_map<std::string, SymbolInfo>& getTable() const {
        return table;
    }

    // Tenta di definire un nuovo simbolo. Ritorna false se esiste già.
    bool define(const std::string& name, const SymbolInfo& info) {
        if (table.find(name) != table.end()) {
            return false; // Errore: Già definito
        }
        table[name] = info;
        return true;
    }

    // ATTENZIONE: Ritorna un puntatore interno alla mappa.
    // NON memorizzare questo puntatore a lungo termine se la tabella viene modificata.
    // Può essere invalidato da un rehash della unordered_map.
    SymbolInfo* lookup(const std::string& name) {
        auto it = table.find(name);
        if (it != table.end()) {
            return &it->second; // Ritorna l'indirizzo della struct salvata
        }
        return nullptr; // Segnala "Non trovato"
    }

    // Debug: stampa tabella
    void printTable() const {
        std::cout << "--- SYMBOL TABLE ---" << std::endl;
        for (const auto& pair : table) {
            // 1. Stampa Nome e Tipo principale
            std::cout << pair.first << " : " << typeToString(pair.second.type);

            // 2. Se è una funzione, stampa anche la lista dei parametri
            if (pair.second.isFunction) {
                std::cout << " : Params(";
                // Ciclo su ogni parametro del vettore
                for (size_t i = 0; i < pair.second.paramTypes.size(); ++i) {
                    std::cout << typeToString(pair.second.paramTypes[i]);
                    // Aggiungi una virgola se non è l'ultimo
                    if (i < pair.second.paramTypes.size() - 1) {
                        std::cout << ", ";
                    }
                }
                std::cout << ")";
            }

            std::cout << std::endl;
        }
        std::cout << "--------------------" << std::endl;
    }
};