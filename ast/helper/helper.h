//
// Created by Giuseppe Gambardella on 01/12/25.
//

#ifndef LITTLEBUTTERFLY_COMPILER_HELPER_H
#define LITTLEBUTTERFLY_COMPILER_HELPER_H
#include <string>
#include "../nodes/TypeNode.hpp"


inline std::string typeToString(BasicType type) {
    // funzione helper per convertire l'enum BasicType in una stringa
    switch (type) {
        case BasicType::INT:    return "Integer"; // Adatta i nomi ai casi
        case BasicType::DOUBLE:  return "Double";
        case BasicType::CHAR:   return "Char";
        case BasicType::BOOL:   return "Boolean";
        case BasicType::STRING: return "String";
        case BasicType::VOID:   return "Void";
        default:                return "Unknown";
    }
}

#endif //LITTLEBUTTERFLY_COMPILER_HELPER_H