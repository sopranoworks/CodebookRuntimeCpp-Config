/*
 * Lexer.h
 *
 * This file is part of the CodebookRuntimeCpp-Config project.
 *
 * Copyright (c) 2026 Sopranoworks, Osamu Takahashi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace CodebookRuntime::Config {

enum class TokenType {
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Colon,
    Assign,
    PlusAssign,
    Comma,
    Newline,
    QuotedString,
    UnquotedString,
    Number,
    Substitution,
    EndOfFile,
};

struct Token {
    TokenType   type;
    std::string value;
};

class Lexer {
public:
    explicit Lexer(std::string_view input) : input_(input), pos_(0) {}

    std::optional<std::vector<Token>> tokenize() {
        std::vector<Token> tokens;

        while (pos_ < input_.size()) {
            skip_inline_whitespace();
            if (pos_ >= input_.size()) break;

            char c = input_[pos_];

            // Comments
            if (c == '#') { skip_line(); continue; }
            if (c == '/' && peek(1) == '/') { skip_line(); continue; }

            // Newlines (HOCON newline-as-separator rule)
            if (c == '\r') {
                if (peek(1) == '\n') ++pos_;   // CRLF -> single Newline
                tokens.push_back({TokenType::Newline, "\n"});
                ++pos_;
                continue;
            }
            if (c == '\n') {
                tokens.push_back({TokenType::Newline, "\n"});
                ++pos_;
                continue;
            }

            // Single-character symbols
            if (c == '{') { tokens.push_back({TokenType::LBrace,   "{"}); ++pos_; continue; }
            if (c == '}') { tokens.push_back({TokenType::RBrace,   "}"}); ++pos_; continue; }
            if (c == '[') { tokens.push_back({TokenType::LBracket, "["}); ++pos_; continue; }
            if (c == ']') { tokens.push_back({TokenType::RBracket, "]"}); ++pos_; continue; }
            if (c == ':') { tokens.push_back({TokenType::Colon,    ":"}); ++pos_; continue; }
            if (c == '=') { tokens.push_back({TokenType::Assign,   "="}); ++pos_; continue; }
            if (c == ',') { tokens.push_back({TokenType::Comma,    ","}); ++pos_; continue; }

            // += (bare '+' without '=' is invalid)
            if (c == '+') {
                if (peek(1) != '=') return std::nullopt;
                tokens.push_back({TokenType::PlusAssign, "+="});
                pos_ += 2;
                continue;
            }

            // Substitution ${
            if (c == '$') {
                if (peek(1) != '{') return std::nullopt;
                tokens.push_back({TokenType::Substitution, "${"});
                pos_ += 2;
                continue;
            }

            // Quoted string
            if (c == '"') {
                auto tok = lex_quoted();
                if (!tok) return std::nullopt;
                tokens.push_back(std::move(*tok));
                continue;
            }

            // Number: digit, or '-' followed by digit
            if (std::isdigit(static_cast<unsigned char>(c)) ||
                (c == '-' && std::isdigit(static_cast<unsigned char>(peek(1))))) {
                auto tok = lex_number();
                if (!tok) return std::nullopt;
                tokens.push_back(std::move(*tok));
                continue;
            }

            // Unquoted string
            if (!is_reserved(c)) {
                auto tok = lex_unquoted();
                if (!tok) return std::nullopt;
                tokens.push_back(std::move(*tok));
                continue;
            }

            return std::nullopt;   // unrecognised character
        }

        tokens.push_back({TokenType::EndOfFile, ""});
        return tokens;
    }

private:
    std::string_view input_;
    std::size_t      pos_;

    char peek(std::size_t offset) const {
        std::size_t idx = pos_ + offset;
        return idx < input_.size() ? input_[idx] : '\0';
    }

    void skip_inline_whitespace() {
        while (pos_ < input_.size() &&
               (input_[pos_] == ' ' || input_[pos_] == '\t'))
            ++pos_;
    }

    void skip_line() {
        while (pos_ < input_.size() &&
               input_[pos_] != '\n' && input_[pos_] != '\r')
            ++pos_;
    }

    // Characters that terminate an unquoted string or cannot start one
    bool is_reserved(char c) const {
        switch (c) {
            case '{': case '}': case '[': case ']':
            case ':': case '=': case ',': case '+':
            case '#': case '"': case '$':
            case ' ': case '\t': case '\n': case '\r':
            case '\0':
                return true;
            default:
                return false;
        }
    }

    std::optional<Token> lex_quoted() {
        ++pos_;   // consume opening '"'
        std::string value;
        while (pos_ < input_.size()) {
            char c = input_[pos_];
            if (c == '"') { ++pos_; return Token{TokenType::QuotedString, std::move(value)}; }
            if (c == '\\') {
                ++pos_;
                if (pos_ >= input_.size()) return std::nullopt;
                switch (input_[pos_]) {
                    case '"':  value += '"';  break;
                    case '\\': value += '\\'; break;
                    case 'n':  value += '\n'; break;
                    case 't':  value += '\t'; break;
                    case 'r':  value += '\r'; break;
                    default:   value += input_[pos_]; break;
                }
            } else {
                value += c;
            }
            ++pos_;
        }
        return std::nullopt;   // unterminated string
    }

    std::optional<Token> lex_number() {
        std::size_t start = pos_;
        if (input_[pos_] == '-') ++pos_;
        while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_])))
            ++pos_;
        if (pos_ < input_.size() && input_[pos_] == '.') {
            ++pos_;
            while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_])))
                ++pos_;
        }
        return Token{TokenType::Number, std::string(input_.substr(start, pos_ - start))};
    }

    std::optional<Token> lex_unquoted() {
        std::size_t start = pos_;
        while (pos_ < input_.size() && !is_reserved(input_[pos_]))
            ++pos_;
        if (pos_ == start) return std::nullopt;
        return Token{TokenType::UnquotedString, std::string(input_.substr(start, pos_ - start))};
    }
};

} // namespace CodebookRuntime::Config
