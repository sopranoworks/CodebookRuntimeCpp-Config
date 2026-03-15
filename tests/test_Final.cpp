#include <CodebookRuntime/Config/Config.h>

#include <fstream>
#include <gtest/gtest.h>

using namespace CodebookRuntime::Config;

// Test 1: Basic substitution  { x: 10, y: ${x} }  →  y == 10
TEST(FinalTest, BasicSubstitution) {
    auto result = Config::parse(R"({ x: 10, y: ${x} })");
    ASSERT_TRUE(result.has_value());

    auto y = result->get("y");
    ASSERT_TRUE(y.has_value());
    EXPECT_EQ(y->as<int64_t>(), std::optional<int64_t>{10});
}

// Test 2: Forward reference  { a: ${b}, b: 1 }  →  a == 1
TEST(FinalTest, ForwardReference) {
    auto result = Config::parse(R"({ a: ${b}, b: 1 })");
    ASSERT_TRUE(result.has_value());

    auto a = result->get("a");
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(a->as<int64_t>(), std::optional<int64_t>{1});
}

// Test 3: Path substitution  { a: { b: 2 }, c: ${a.b} }  →  c == 2
TEST(FinalTest, PathSubstitution) {
    auto result = Config::parse(R"({ a: { b: 2 }, c: ${a.b} })");
    ASSERT_TRUE(result.has_value());

    auto c = result->get("c");
    ASSERT_TRUE(c.has_value());
    EXPECT_EQ(c->as<int64_t>(), std::optional<int64_t>{2});
}

// Test 4: File loading – write a small .conf file and read it back
TEST(FinalTest, FileLoading) {
    const std::string path = "/tmp/codebook_test.conf";
    {
        std::ofstream f(path);
        ASSERT_TRUE(f.is_open());
        f << "x = 42\n";
        f << "name = hello\n";
        f << "nested { key = world }\n";
    }

    auto result = Config::from_file(path);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get("x")->as<int64_t>(),       std::optional<int64_t>{42});
    EXPECT_EQ(result->get("name")->as<std::string>(), std::optional<std::string>{"hello"});
    EXPECT_EQ(result->get("nested.key")->as<std::string>(), std::optional<std::string>{"world"});
}

// Test 5: File not found returns nullopt
TEST(FinalTest, FileNotFound) {
    EXPECT_FALSE(Config::from_file("/nonexistent/path/config.conf").has_value());
}

// Test 6: Optional substitution ${?missing}  →  null (monostate), no error
TEST(FinalTest, OptionalSubstitutionMissing) {
    auto result = Config::parse(R"({ a: ${?no_such_key} })");
    ASSERT_TRUE(result.has_value());

    auto a = result->get("a");
    ASSERT_TRUE(a.has_value());
    EXPECT_TRUE(a->as<std::monostate>().has_value());
}

// Test 7: Required substitution missing  →  nullopt
TEST(FinalTest, RequiredSubstitutionMissing) {
    EXPECT_FALSE(Config::parse(R"({ a: ${no_such_key} })").has_value());
}

// Test 8: Substitution resolved value is itself used in a path lookup
TEST(FinalTest, SubstitutionInNestedPath) {
    // a.b is defined; c and d both reference it via different substitutions
    auto result = Config::parse(
        "a { b = 99 }\n"
        "c = ${a.b}\n"
        "d = ${c}\n");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get("c")->as<int64_t>(), std::optional<int64_t>{99});
    EXPECT_EQ(result->get("d")->as<int64_t>(), std::optional<int64_t>{99});
}
