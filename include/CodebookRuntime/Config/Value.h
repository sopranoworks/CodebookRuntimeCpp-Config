#pragma once

#include "Common.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace CodebookRuntime::Config {

class Value {
public:
    // Represents an unresolved ${path} or ${?path} substitution.
    struct Placeholder {
        std::string path;
        bool        optional = false;
    };

    using Storage = std::variant<
        std::monostate,
        bool,
        int64_t,
        double,
        std::string,
        Array,
        Object,
        Placeholder
    >;

    Value() = default;
    Value(std::monostate v)  : storage_(v) {}
    Value(bool v)            : storage_(v) {}
    Value(int64_t v)         : storage_(v) {}
    Value(int v)             : storage_(static_cast<int64_t>(v)) {}
    Value(double v)          : storage_(v) {}
    Value(std::string v)     : storage_(std::move(v)) {}
    Value(const char* v)     : storage_(std::string(v)) {}
    Value(Array v)           : storage_(std::move(v)) {}
    Value(Object v)          : storage_(std::move(v)) {}
    Value(Placeholder v)     : storage_(std::move(v)) {}

    // Returns value if the stored type exactly matches T.
    template <typename T>
    std::optional<T> as() const {
        if (const T* p = std::get_if<T>(&storage_)) {
            return *p;
        }
        return std::nullopt;
    }

    // Resolves a dot-separated path, e.g. "a.b.c".
    std::optional<Value> get(std::string_view path) const {
        const Object* obj = std::get_if<Object>(&storage_);
        if (!obj) return std::nullopt;

        auto dot = path.find('.');
        std::string_view key  = path.substr(0, dot);
        std::string key_str(key);

        auto it = obj->find(key_str);
        if (it == obj->end()) return std::nullopt;

        if (dot == std::string_view::npos) {
            return it->second;
        }
        return it->second.get(path.substr(dot + 1));
    }

    // Deep merges other into this.
    // If both are Objects: merge keys recursively (other wins on conflict).
    // Otherwise: overwrite with other.
    void merge(Value&& other) {
        Object* lhs = std::get_if<Object>(&storage_);
        Object* rhs = std::get_if<Object>(&other.storage_);

        if (lhs && rhs) {
            for (auto& [k, v] : *rhs) {
                auto it = lhs->find(k);
                if (it == lhs->end()) {
                    (*lhs)[k] = std::move(v);
                } else {
                    it->second.merge(std::move(v));
                }
            }
        } else {
            storage_ = std::move(other.storage_);
        }
    }

    Storage&       raw()       { return storage_; }
    const Storage& raw() const { return storage_; }

private:
    Storage storage_;
};

} // namespace CodebookRuntime::Config
