#include <CodebookRuntime/Config/Config.h>

#include <gtest/gtest.h>

using namespace CodebookRuntime::Config;

// Test 1: Simple JSON-like object  { "a": 1, "b": "hello" }
TEST(ParserTest, SimpleJsonObject) {
    auto result = Config::parse(R"({ "a": 1, "b": "hello" })");
    ASSERT_TRUE(result.has_value());

    auto a = result->get("a");
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(a->as<int64_t>(), std::optional<int64_t>{1});

    auto b = result->get("b");
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(b->as<std::string>(), std::optional<std::string>{"hello"});
}

// Test 2: Nested path assignment  root.sub.key = value
TEST(ParserTest, NestedPathAssignment) {
    auto result = Config::parse("root.sub.key = value");
    ASSERT_TRUE(result.has_value());

    auto val = result->get("root.sub.key");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val->as<std::string>(), std::optional<std::string>{"value"});
}

// Test 3: Object merging  { a: { x: 1 }, a: { y: 2 } }  →  { a: { x:1, y:2 } }
TEST(ParserTest, ObjectMerging) {
    auto result = Config::parse(R"({ a: { x: 1 }, a: { y: 2 } })");
    ASSERT_TRUE(result.has_value());

    auto x = result->get("a.x");
    ASSERT_TRUE(x.has_value());
    EXPECT_EQ(x->as<int64_t>(), std::optional<int64_t>{1});

    auto y = result->get("a.y");
    ASSERT_TRUE(y.has_value());
    EXPECT_EQ(y->as<int64_t>(), std::optional<int64_t>{2});
}

// Test 4: Array appending with +=  list = [1], list += 2  →  list = [1, 2]
TEST(ParserTest, ArrayAppendPlusAssign) {
    auto result = Config::parse("list = [1], list += 2");
    ASSERT_TRUE(result.has_value());

    auto list_val = result->get("list");
    ASSERT_TRUE(list_val.has_value());

    const auto* arr = std::get_if<Array>(&list_val->raw());
    ASSERT_NE(arr, nullptr);
    ASSERT_EQ(arr->size(), 2u);
    EXPECT_EQ((*arr)[0].as<int64_t>(), std::optional<int64_t>{1});
    EXPECT_EQ((*arr)[1].as<int64_t>(), std::optional<int64_t>{2});
}

// Test 5: Boolean and null literals
TEST(ParserTest, BoolAndNullLiterals) {
    auto result = Config::parse("flag = true\nempty = null\noff = false");
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result->get("flag")->as<bool>(),        std::optional<bool>{true});
    EXPECT_EQ(result->get("off")->as<bool>(),         std::optional<bool>{false});
    EXPECT_TRUE(result->get("empty")->as<std::monostate>().has_value());
}

// Test 6: Array of mixed scalars
TEST(ParserTest, ArrayOfScalars) {
    auto result = Config::parse(R"(nums = [10, 20, 30])");
    ASSERT_TRUE(result.has_value());

    auto v = result->get("nums");
    ASSERT_TRUE(v.has_value());

    const auto* arr = std::get_if<Array>(&v->raw());
    ASSERT_NE(arr, nullptr);
    ASSERT_EQ(arr->size(), 3u);
    EXPECT_EQ((*arr)[0].as<int64_t>(), std::optional<int64_t>{10});
    EXPECT_EQ((*arr)[1].as<int64_t>(), std::optional<int64_t>{20});
    EXPECT_EQ((*arr)[2].as<int64_t>(), std::optional<int64_t>{30});
}

// Test 7: Error – malformed input returns nullopt
TEST(ParserTest, MalformedInputReturnsNullopt) {
    EXPECT_FALSE(Config::parse("= broken").has_value());
}
