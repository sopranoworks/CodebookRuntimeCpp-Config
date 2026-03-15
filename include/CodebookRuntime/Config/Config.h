#pragma once

#include "Lexer.h"
#include "Parser.h"
#include "Value.h"

#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

namespace CodebookRuntime::Config {

struct Config {
    // Tokenize, parse, then resolve all substitutions.
    // Returns nullopt on any syntax or resolution error.
    static std::optional<Value> parse(std::string_view input) {
        auto tokens = Lexer(input).tokenize();
        if (!tokens) return std::nullopt;

        auto tree = Parser(std::move(*tokens)).parse();
        if (!tree) return std::nullopt;

        return resolve(std::move(*tree));
    }

    // Reads the file at 'path' and passes its contents to parse().
    // Returns nullopt if the file cannot be opened or parsing fails.
    static std::optional<Value> from_file(const std::string& path) {
        std::ifstream file(path);
        if (!file) return std::nullopt;
        std::string content(std::istreambuf_iterator<char>(file),
                            std::istreambuf_iterator<char>{});
        return parse(content);
    }

    // Performs a full substitution-resolution pass over an already-parsed tree.
    // Returns nullopt if a required substitution cannot be resolved.
    static std::optional<Value> resolve(Value root) {
        if (!resolve_node(root, root, 0)) return std::nullopt;
        return root;
    }

private:
    // Recursively replaces every Placeholder node with the value found at
    // its path in 'root'.  'depth' guards against circular substitutions.
    static bool resolve_node(Value& node, const Value& root, int depth) {
        if (depth > 256) return false;   // circular substitution guard

        if (auto* ph = std::get_if<Value::Placeholder>(&node.raw())) {
            auto found = root.get(ph->path);
            if (!found) {
                if (ph->optional) { node = Value{}; return true; }
                return false;   // required substitution missing
            }
            node = *found;   // replace (copy of looked-up value)
            return resolve_node(node, root, depth + 1);
        }

        if (auto* obj = std::get_if<Object>(&node.raw())) {
            for (auto& [k, v] : *obj)
                if (!resolve_node(v, root, depth)) return false;
        } else if (auto* arr = std::get_if<Array>(&node.raw())) {
            for (auto& elem : *arr)
                if (!resolve_node(elem, root, depth)) return false;
        }
        return true;
    }
};

} // namespace CodebookRuntime::Config
