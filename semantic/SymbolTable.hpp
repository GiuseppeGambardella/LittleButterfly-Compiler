#pragma once
#include <string>
#include <unordered_map>
#include <iostream>
#include "../ast/nodes/expression/TypeNode.hpp"
#include <vector>
#include "helper/helper.h"

// info associeted to a symbol
struct SymbolInfo {
    BasicType type;
    bool isFunction;
    std::vector<BasicType> paramTypes;
};

class SymbolTable {
private:
    std::unordered_map<std::string, SymbolInfo> table;

public:

    const std::unordered_map<std::string, SymbolInfo>& getTable() const {
        return table;
    }

    // tries to define a new symbol. Returns false if it already exists.
    bool define(const std::string& name, const SymbolInfo& info) {
        if (table.find(name) != table.end()) {
            return false;
        }
        table[name] = info;
        return true;
    }

    SymbolInfo* lookup(const std::string& name) {
        auto it = table.find(name);
        if (it != table.end()) {
            return &it->second; // returns a pointer to the found SymbolInfo
        }
        return nullptr; // not found
    }

    // Debug: stampa tabella
    void printTable() const {
        std::cout << "--- SYMBOL TABLE ---" << std::endl;
        for (const auto& pair : table) {
            std::cout << pair.first << " : " << typeToString(pair.second.type);


            if (pair.second.isFunction) {
                std::cout << " : Params(";
                for (size_t i = 0; i < pair.second.paramTypes.size(); ++i) {
                    std::cout << typeToString(pair.second.paramTypes[i]);
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