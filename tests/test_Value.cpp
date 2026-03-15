#include <CodebookRuntime/Config/Value.h>

#include <gtest/gtest.h>

using namespace CodebookRuntime::Config;

// Test 1: Basic type storage and retrieval
TEST(ValueTest, BasicTypes) {
    Value i(int64_t{42});
    EXPECT_EQ(i.as<int64_t>(), std::optional<int64_t>{42});
    EXPECT_FALSE(i.as<std::string>().has_value());

    Value s(std::string{"hello"});
    EXPECT_EQ(s.as<std::string>(), std::optional<std::string>{"hello"});

    Value b(true);
    EXPECT_EQ(b.as<bool>(), std::optional<bool>{true});

    Value n;
    EXPECT_TRUE(n.as<std::monostate>().has_value());
}

// Test 2: Path lookup
TEST(ValueTest, PathLookup) {
    // Build: { "a": { "b": 100 } }
    Object inner;
    inner["b"] = Value(int64_t{100});

    Object outer;
    outer["a"] = Value(std::move(inner));

    Value root(std::move(outer));

    auto direct = root.get("a");
    ASSERT_TRUE(direct.has_value());

    auto nested = root.get("a.b");
    ASSERT_TRUE(nested.has_value());
    EXPECT_EQ(nested->as<int64_t>(), std::optional<int64_t>{100});

    EXPECT_FALSE(root.get("a.c").has_value());
    EXPECT_FALSE(root.get("x").has_value());
}

// Test 3: Deep merge
// { a:1, b:{x:1} }  merge  { c:2, b:{y:2} }
// expected: { a:1, c:2, b:{x:1, y:2} }
TEST(ValueTest, DeepMerge) {
    Object bLeft;
    bLeft["x"] = Value(int64_t{1});

    Object left;
    left["a"] = Value(int64_t{1});
    left["b"] = Value(std::move(bLeft));

    Object bRight;
    bRight["y"] = Value(int64_t{2});

    Object right;
    right["c"] = Value(int64_t{2});
    right["b"] = Value(std::move(bRight));

    Value lhs(std::move(left));
    Value rhs(std::move(right));

    lhs.merge(std::move(rhs));

    EXPECT_EQ(lhs.get("a")->as<int64_t>(), std::optional<int64_t>{1});
    EXPECT_EQ(lhs.get("c")->as<int64_t>(), std::optional<int64_t>{2});
    EXPECT_EQ(lhs.get("b.x")->as<int64_t>(), std::optional<int64_t>{1});
    EXPECT_EQ(lhs.get("b.y")->as<int64_t>(), std::optional<int64_t>{2});
}
