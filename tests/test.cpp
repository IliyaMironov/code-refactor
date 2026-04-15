#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

using ::testing::HasSubstr;
using ::testing::Not;

static std::string readFile(const fs::path &path) {
    std::ifstream f(path);
    EXPECT_TRUE(f.is_open()) << "Cannot open: " << path;
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string runRefactorTool(const std::string &srcName) {
    fs::path src = fs::path(TESTS_DATA_DIR) / srcName;
    fs::path dst = fs::temp_directory_path() / ("refactor_test_" + srcName);

    fs::copy_file(src, dst, fs::copy_options::overwrite_existing);

    std::string cmd = std::string(REFACTOR_TOOL_PATH)
                    + " " + dst.string()
                    + " -- -std=c++17 2>/dev/null";
    int ret = std::system(cmd.c_str());
    EXPECT_EQ(ret, 0) << "refactor_tool failed on " << srcName;

    return readFile(dst);
}

TEST(NvDtor, AddsVirtualToBaseDestructor) {
    std::string result   = runRefactorTool("test1.cpp");
    std::string expected = readFile(fs::path(TESTS_DATA_DIR) / "test1_ref.cpp");
    EXPECT_EQ(result, expected);
}

TEST(MissingOverride, AddsOverrideToOverriddenMethods) {
    std::string result   = runRefactorTool("test2.cpp");
    std::string expected = readFile(fs::path(TESTS_DATA_DIR) / "test2_ref.cpp");
    EXPECT_EQ(result, expected);
}

TEST(RangeFor, AddsReferenceToConstLoopVar) {
    std::string result   = runRefactorTool("test3.cpp");
    std::string expected = readFile(fs::path(TESTS_DATA_DIR) / "test3_ref.cpp");
    EXPECT_EQ(result, expected);
}

TEST(Combined, AppliesAllThreeRefactorings) {
    std::string result   = runRefactorTool("test4.cpp");
    std::string expected = readFile(fs::path(TESTS_DATA_DIR) / "test4_ref.cpp");
    EXPECT_EQ(result, expected);

    EXPECT_THAT(result, HasSubstr("virtual ~Animal()"));
    EXPECT_THAT(result, HasSubstr("speak() override"));
    EXPECT_THAT(result, HasSubstr("const auto& name"));
}

TEST(NoChanges, LeavesCorrectFileUnchanged) {
    std::string before = readFile(fs::path(TESTS_DATA_DIR) / "test5.cpp");
    std::string result = runRefactorTool("test5.cpp");
    std::string expected = readFile(fs::path(TESTS_DATA_DIR) / "test5_ref.cpp");
    EXPECT_EQ(result, expected);

    EXPECT_EQ(result, before);
}

TEST(DeepInheritance, AddsOverrideAtEveryLevel) {
    std::string result   = runRefactorTool("test6.cpp");
    std::string expected = readFile(fs::path(TESTS_DATA_DIR) / "test6_ref.cpp");
    EXPECT_EQ(result, expected);

    EXPECT_THAT(result, HasSubstr("class Middle"));
    EXPECT_THAT(result, HasSubstr("action() override"));
    EXPECT_THAT(result, HasSubstr("info() override"));
}

TEST(MultipleInheritance, AddsVirtualDtorToBothBases) {
    std::string result   = runRefactorTool("test7.cpp");
    std::string expected = readFile(fs::path(TESTS_DATA_DIR) / "test7_ref.cpp");
    EXPECT_EQ(result, expected);

    EXPECT_THAT(result, HasSubstr("virtual ~Serializable()"));
    EXPECT_THAT(result, HasSubstr("print() override"));
    EXPECT_THAT(result, HasSubstr("serialize() override"));
}

TEST(RangeForContainers, AddsRefForNonFundamentalTypes) {
    std::string result   = runRefactorTool("test8.cpp");
    std::string expected = readFile(fs::path(TESTS_DATA_DIR) / "test8_ref.cpp");
    EXPECT_EQ(result, expected);

    EXPECT_THAT(result, HasSubstr("const auto& p : points"));
    EXPECT_THAT(result, HasSubstr("const auto& entry : dict"));
    EXPECT_THAT(result, HasSubstr("const auto& tag : tags"));
    EXPECT_THAT(result, HasSubstr("const auto id : ids"));
    EXPECT_THAT(result, Not(HasSubstr("const auto& id : ids")));
}

TEST(ConstMethods, AddsOverrideToConstMethods) {
    std::string result   = runRefactorTool("test9.cpp");
    std::string expected = readFile(fs::path(TESTS_DATA_DIR) / "test9_ref.cpp");
    EXPECT_EQ(result, expected);

    EXPECT_THAT(result, HasSubstr("name() const override"));
    EXPECT_THAT(result, HasSubstr("count() const override"));
    EXPECT_THAT(result, HasSubstr("reset() override"));
}

TEST(MultipleDtors, AddsVirtualToMultipleBaseClasses) {
    std::string result   = runRefactorTool("test10.cpp");
    std::string expected = readFile(fs::path(TESTS_DATA_DIR) / "test10_ref.cpp");
    EXPECT_EQ(result, expected);

    EXPECT_THAT(result, HasSubstr("virtual ~Logger()"));
    EXPECT_THAT(result, HasSubstr("virtual ~Handler()"));
    EXPECT_THAT(result, Not(HasSubstr("virtual ~Standalone()")));
}

TEST(ShapeHierarchy, AddsOverrideAcrossMultipleDerived) {
    std::string result   = runRefactorTool("test11.cpp");
    std::string expected = readFile(fs::path(TESTS_DATA_DIR) / "test11_ref.cpp");
    EXPECT_EQ(result, expected);

    EXPECT_THAT(result, HasSubstr("draw() override"));
    EXPECT_THAT(result, HasSubstr("area() const override"));
    EXPECT_THAT(result, HasSubstr("virtual void draw()"));
    EXPECT_THAT(result, HasSubstr("virtual double area()"));
}
