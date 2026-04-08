#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

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
