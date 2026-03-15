#pragma once

#include "Lexer.h"
#include "Value.h"

#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace CodebookRuntime::Config {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : tokens_(std::move(tokens)), pos_(0) {}

    // Parses the token stream as a HOCON root object.
    // The root can be a bare object body or a single braced { } object.
    std::optional<Value> parse() {
        skip_sep();
        Value result;
        if (peek_is(TokenType::LBrace)) {
            auto v = parse_object();
            if (!v) return std::nullopt;
            result = std::move(*v);
        } else {
            auto v = parse_object_body();
            if (!v) return std::nullopt;
            result = std::move(*v);
        }
        skip_sep();
        if (!at_eof()) return std::nullopt;
        return result;
    }

private:
    std::vector<Token> tokens_;
    std::size_t pos_;

    // ── Token stream helpers ─────────────────────────────────────────────────

    bool at_eof() const {
        return pos_ >= tokens_.size() ||
               tokens_[pos_].type == TokenType::EndOfFile;
    }

    const Token& cur() const { return tokens_[pos_]; }

    bool peek_is(TokenType t) const {
        return !at_eof() && cur().type == t;
    }

    void advance() { if (!at_eof()) ++pos_; }

    // Skip commas and newlines (both serve as value separators in HOCON)
    void skip_sep() {
        while (!at_eof() &&
               (cur().type == TokenType::Newline ||
                cur().type == TokenType::Comma))
            advance();
    }

    bool is_key_token() const {
        if (at_eof()) return false;
        auto t = cur().type;
        return t == TokenType::QuotedString || t == TokenType::UnquotedString;
    }

    // ── Key parsing ──────────────────────────────────────────────────────────

    // Returns a path like ["a","b","c"] for both "a.b.c" (unquoted) and "a" (quoted).
    std::optional<std::vector<std::string>> parse_key() {
        if (!is_key_token()) return std::nullopt;
        std::vector<std::string> parts;
        if (cur().type == TokenType::QuotedString) {
            parts.push_back(cur().value);
        } else {
            // Unquoted string may embed a dot-separated path (e.g. "a.b.c").
            std::string_view sv = cur().value;
            while (!sv.empty()) {
                auto dot = sv.find('.');
                parts.emplace_back(sv.substr(0, dot));
                if (dot == std::string_view::npos) break;
                sv.remove_prefix(dot + 1);
            }
        }
        advance();
        return parts;
    }

    // ── Path builder ─────────────────────────────────────────────────────────

    // Wraps leaf in nested objects for the given path.
    // path=["a","b"] leaf=V  →  {a:{b:V}}
    static Value build_path(const std::vector<std::string>& path, Value leaf) {
        Value cur = std::move(leaf);
        for (int i = static_cast<int>(path.size()) - 1; i >= 0; --i) {
            Object obj;
            obj[path[static_cast<std::size_t>(i)]] = std::move(cur);
            cur = Value(std::move(obj));
        }
        return cur;
    }

    // ── Recursive descent ────────────────────────────────────────────────────

    std::optional<Value> parse_object_body() {
        Value result(Object{});
        skip_sep();

        while (!at_eof() && !peek_is(TokenType::RBrace)) {
            if (!is_key_token()) return std::nullopt;

            auto key_path = parse_key();
            if (!key_path) return std::nullopt;

            bool is_plus = false;
            if (peek_is(TokenType::Assign) || peek_is(TokenType::Colon)) {
                advance(); skip_sep();
            } else if (peek_is(TokenType::PlusAssign)) {
                is_plus = true;
                advance(); skip_sep();
            } else if (!peek_is(TokenType::LBrace)) {
                return std::nullopt;
            }
            // (If next token is LBrace we fall through; parse_value will consume it.)

            auto rhs = parse_value();
            if (!rhs) return std::nullopt;

            if (is_plus) {
                // += is only supported for single-segment keys.
                if (key_path->size() != 1) return std::nullopt;
                const std::string& k = (*key_path)[0];

                // Build new array from existing value (if any) + rhs.
                Array arr;
                if (const auto* obj_ptr = std::get_if<Object>(&result.raw())) {
                    auto it = obj_ptr->find(k);
                    if (it != obj_ptr->end()) {
                        if (const auto* ea = std::get_if<Array>(&it->second.raw()))
                            arr = *ea;
                        else
                            arr.push_back(it->second);
                    }
                }
                if (const auto* ra = std::get_if<Array>(&rhs->raw()))
                    for (const auto& e : *ra) arr.push_back(e);
                else
                    arr.push_back(std::move(*rhs));

                Value wrapped = build_path(*key_path, Value(std::move(arr)));
                result.merge(std::move(wrapped));
            } else {
                Value nested = build_path(*key_path, std::move(*rhs));
                result.merge(std::move(nested));
            }

            skip_sep();
        }

        return result;
    }

    std::optional<Value> parse_object() {
        if (!peek_is(TokenType::LBrace)) return std::nullopt;
        advance();     // consume '{'
        skip_sep();

        auto body = parse_object_body();
        if (!body) return std::nullopt;

        skip_sep();
        if (!peek_is(TokenType::RBrace)) return std::nullopt;
        advance();     // consume '}'
        return body;
    }

    std::optional<Value> parse_array() {
        if (!peek_is(TokenType::LBracket)) return std::nullopt;
        advance();     // consume '['
        skip_sep();

        Array arr;
        while (!at_eof() && !peek_is(TokenType::RBracket)) {
            auto val = parse_value();
            if (!val) return std::nullopt;
            arr.push_back(std::move(*val));
            skip_sep();
        }

        if (!peek_is(TokenType::RBracket)) return std::nullopt;
        advance();     // consume ']'
        return Value(std::move(arr));
    }

    std::optional<Value> parse_value() {
        if (at_eof()) return std::nullopt;

        switch (cur().type) {
            case TokenType::LBrace:   return parse_object();
            case TokenType::LBracket: return parse_array();

            case TokenType::QuotedString: {
                Value v(cur().value);
                advance();
                return v;
            }
            case TokenType::UnquotedString: {
                const std::string& s = cur().value;
                if (s == "true")  { advance(); return Value(true); }
                if (s == "false") { advance(); return Value(false); }
                if (s == "null")  { advance(); return Value(std::monostate{}); }
                Value v(s);
                advance();
                return v;
            }
            case TokenType::Number: {
                const std::string& s = cur().value;
                char* end = nullptr;
                const int64_t ival = std::strtoll(s.c_str(), &end, 10);
                if (end && *end == '\0') { advance(); return Value(ival); }
                const double dval = std::strtod(s.c_str(), &end);
                if (end && *end == '\0') { advance(); return Value(dval); }
                return std::nullopt;
            }
            case TokenType::Substitution: {
                advance();   // consume '${'
                if (at_eof() || cur().type != TokenType::UnquotedString)
                    return std::nullopt;
                std::string path = cur().value;
                advance();   // consume path token
                bool optional = false;
                if (!path.empty() && path[0] == '?') {
                    optional = true;
                    path = path.substr(1);
                }
                if (!peek_is(TokenType::RBrace)) return std::nullopt;
                advance();   // consume '}'
                return Value(Value::Placeholder{std::move(path), optional});
            }
            default:
                return std::nullopt;
        }
    }
};

} // namespace CodebookRuntime::Config
