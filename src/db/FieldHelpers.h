#pragma once
#include <drogon/orm/Field.h>
#include <string>

namespace blueduck {

template <typename T>
inline T fieldOr(const drogon::orm::Field& f, const T& def) {
    return f.isNull() ? def : f.as<T>();
}

inline std::string fieldOrEmpty(const drogon::orm::Field& f) {
    return f.isNull() ? std::string{} : f.as<std::string>();
}

} // namespace blueduck
