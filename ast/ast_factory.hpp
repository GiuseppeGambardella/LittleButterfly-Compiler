#pragma once
#include <memory>
#include <utility>

template <typename T, typename... Args>
std::unique_ptr<T> make_node(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}
// Usage example:
// auto binaryOpNode = make_node<BinaryOpNode>("+", std::move(numberNode), std::move(stringNode));