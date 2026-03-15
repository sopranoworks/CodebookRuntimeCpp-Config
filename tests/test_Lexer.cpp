#include <CodebookRuntime/Config/Lexer.h>

#include <gtest/gtest.h>

using namespace CodebookRuntime::Config;

// Helper: tokenize and strip Newline + EndOfFile tokens for concise assertions.
static std::vector<Token> tokenize_clean(std::string_view input) {
    auto result = Lexer(input).tokenize();
    EXPECT_TRUE(result.has_value());
    if (!result) return {};
    std::vector<Token> out;
    for (auto& t : *result) {
        if (t.type != TokenType::Newline && t.type != TokenType::EndOfFile)
            out.push_back(t);
    }
    return out;
}

// Test 1: Standard JSON-like tokens  { "a": 1 }
TEST(LexerTest, JsonLikeTokens) {
    auto tokens = tokenize_clean(R"({ "a": 1 })");

    ASSERT_EQ(tokens.size(), 5u);
    EXPECT_EQ(tokens[0].type,  TokenType::LBrace);
    EXPECT_EQ(tokens[1].type,  TokenType::QuotedString);
    EXPECT_EQ(tokens[1].value, "a");
    EXPECT_EQ(tokens[2].type,  TokenType::Colon);
    EXPECT_EQ(tokens[3].type,  TokenType::Number);
    EXPECT_EQ(tokens[3].value, "1");
    EXPECT_EQ(tokens[4].type,  TokenType::RBrace);
}

// Test 2: HOCON unquoted keys and values  a = b
TEST(LexerTest, UnquotedKeyValue) {
    auto tokens = tokenize_clean("a = b");

    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type,  TokenType::UnquotedString);
    EXPECT_EQ(tokens[0].value, "a");
    EXPECT_EQ(tokens[1].type,  TokenType::Assign);
    EXPECT_EQ(tokens[2].type,  TokenType::UnquotedString);
    EXPECT_EQ(tokens[2].value, "b");
}

// Test 3: += is a single PlusAssign token  a += b
TEST(LexerTest, PlusAssignOperator) {
    auto tokens = tokenize_clean("a += b");

    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type,  TokenType::UnquotedString);
    EXPECT_EQ(tokens[0].value, "a");
    EXPECT_EQ(tokens[1].type,  TokenType::PlusAssign);
    EXPECT_EQ(tokens[1].value, "+=");
    EXPECT_EQ(tokens[2].type,  TokenType::UnquotedString);
    EXPECT_EQ(tokens[2].value, "b");
}

// Test 4a: Hash comments are stripped  a = 1 # comment
TEST(LexerTest, HashCommentStripped) {
    auto tokens = tokenize_clean("a = 1 # this is ignored");

    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type,  TokenType::UnquotedString);
    EXPECT_EQ(tokens[0].value, "a");
    EXPECT_EQ(tokens[1].type,  TokenType::Assign);
    EXPECT_EQ(tokens[2].type,  TokenType::Number);
    EXPECT_EQ(tokens[2].value, "1");
}

// Test 4b: Double-slash comments are stripped  a = 1 // comment
TEST(LexerTest, SlashCommentStripped) {
    auto tokens = tokenize_clean("a = 1 // this is also ignored");

    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type,  TokenType::UnquotedString);
    EXPECT_EQ(tokens[0].value, "a");
    EXPECT_EQ(tokens[1].type,  TokenType::Assign);
    EXPECT_EQ(tokens[2].type,  TokenType::Number);
    EXPECT_EQ(tokens[2].value, "1");
}

// Test 5: Multi-line input with newline tokens present
TEST(LexerTest, NewlinesEmitted) {
    auto result = Lexer("a = 1\nb = 2").tokenize();
    ASSERT_TRUE(result.has_value());

    bool found_newline = false;
    for (auto& t : *result)
        if (t.type == TokenType::Newline) { found_newline = true; break; }
    EXPECT_TRUE(found_newline);
}

// Test 6: Substitution token ${
TEST(LexerTest, SubstitutionToken) {
    auto tokens = tokenize_clean("${");

    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type,  TokenType::Substitution);
    EXPECT_EQ(tokens[0].value, "${");
}

// Test 7: Error cases return nullopt
TEST(LexerTest, ErrorOnBareplus) {
    EXPECT_FALSE(Lexer("a + b").tokenize().has_value());
}

TEST(LexerTest, ErrorOnUnterminatedString) {
    EXPECT_FALSE(Lexer(R"("unterminated)").tokenize().has_value());
}
