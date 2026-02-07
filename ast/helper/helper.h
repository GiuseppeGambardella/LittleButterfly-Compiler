#pragma once
#include <string>
#include "../nodes/expression/TypeNode.hpp"


inline std::string typeToString(BasicType type) {
    // helper function to convert BasicType to string
    switch (type) {
        case BasicType::INT:    return "Integer"; // Adatta i nomi ai casi
        case BasicType::DOUBLE:  return "Double";
        case BasicType::CHAR:   return "Char";
        case BasicType::BOOL:   return "Boolean";
        case BasicType::STRING: return "String";
        case BasicType::VOID:   return "Void";
        case BasicType::ERROR:  return "Error";
        default:                return "Unknown";
    }
}