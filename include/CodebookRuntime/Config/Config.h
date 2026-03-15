#pragma once

#include "Lexer.h"
#include "Parser.h"
#include "Value.h"

#include <optional>
#include <string_view>

namespace CodebookRuntime::Config {

// Main entry point: tokenizes input then parses it into a Value tree.
struct Config {
    static std::optional<Value> parse(std::string_view input) {
        auto tokens = Lexer(input).tokenize();
        if (!tokens) return std::nullopt;
        return Parser(std::move(*tokens)).parse();
    }
};

} // namespace CodebookRuntime::Config
