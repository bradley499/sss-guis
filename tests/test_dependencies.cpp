#include <gtest/gtest.h>
#include "dependencies.hpp"
#include "test_helpers.hpp"
#include <algorithm>
#include <set>

using namespace sss::guis;

/**
 * @brief Test fixture for the Dependencies class
 */
class DependenciesTest : public ::testing::Test
{
protected:
    TemporaryDirectory temporary_directory{"dependencies_test"};
};

/**
 * @brief Test single exact file resolution
 */
TEST_F(DependenciesTest, ExactExistingFile)
{
    auto const file = temporary_directory.create_file("test.txt", "hello world");
    Dependencies dependencies(file);
    auto const paths = dependencies.paths();

    ASSERT_EQ(paths.size(), 1u);
    EXPECT_EQ(paths[0], file);
}

/**
 * @brief Test single exact directory resolution
 */
TEST_F(DependenciesTest, ExactExistingDirectory)
{
    auto const directory = temporary_directory.create_directory("subdirectory");
    Dependencies dependencies(directory);
    auto const paths = dependencies.paths();

    ASSERT_EQ(paths.size(), 1u);
    EXPECT_EQ(paths[0], directory);
}

/**
 * @brief Test wildcard pattern matching single star extension
 */
TEST_F(DependenciesTest, WildcardSingleStarExtension)
{
    auto const matching_file_a = temporary_directory.create_file("first.txt", "1");
    auto const matching_file_b = temporary_directory.create_file("second.txt", "2");
    temporary_directory.create_file("third.log", "3");

    Dependencies dependencies(temporary_directory.path() / "*.txt");
    auto const paths = dependencies.paths();

    ASSERT_EQ(paths.size(), 2u);
    std::set<std::filesystem::path> const path_set(paths.begin(), paths.end());
    EXPECT_TRUE(path_set.count(matching_file_a));
    EXPECT_TRUE(path_set.count(matching_file_b));
}

/**
 * @brief Test wildcard pattern with prefix and suffix
 */
TEST_F(DependenciesTest, WildcardPrefixAndSuffix)
{
    auto const matching_file_a = temporary_directory.create_file("prefix_1_suffix.txt", "1");
    auto const matching_file_b = temporary_directory.create_file("prefix_abc_suffix.txt", "2");
    temporary_directory.create_file("prefix_1_other.txt", "3");
    temporary_directory.create_file("other_prefix_1_suffix.txt", "4");

    Dependencies dependencies(temporary_directory.path() / "prefix_*_suffix.txt");
    auto const paths = dependencies.paths();

    ASSERT_EQ(paths.size(), 2u);
    std::set<std::filesystem::path> const path_set(paths.begin(), paths.end());
    EXPECT_TRUE(path_set.count(matching_file_a));
    EXPECT_TRUE(path_set.count(matching_file_b));
}

/**
 * @brief Test wildcard pattern with question mark single character
 */
TEST_F(DependenciesTest, WildcardQuestionMark)
{
    auto const matching_file_a = temporary_directory.create_file("file1.txt", "1");
    auto const matching_file_b = temporary_directory.create_file("fileA.txt", "2");
    temporary_directory.create_file("file10.txt", "3");
    temporary_directory.create_file("file.txt", "4");

    Dependencies dependencies(temporary_directory.path() / "file?.txt");
    auto const paths = dependencies.paths();

    ASSERT_EQ(paths.size(), 2u);
    std::set<std::filesystem::path> const path_set(paths.begin(), paths.end());
    EXPECT_TRUE(path_set.count(matching_file_a));
    EXPECT_TRUE(path_set.count(matching_file_b));
}

/**
 * @brief Test wildcard pattern with multiple question marks
 */
TEST_F(DependenciesTest, WildcardMultipleQuestionMarks)
{
    auto const matching_file_a = temporary_directory.create_file("file12.dat", "1");
    auto const matching_file_b = temporary_directory.create_file("fileAB.dat", "2");
    temporary_directory.create_file("file1.dat", "3");
    temporary_directory.create_file("file123.dat", "4");

    Dependencies dependencies(temporary_directory.path() / "file??.dat");
    auto const paths = dependencies.paths();

    ASSERT_EQ(paths.size(), 2u);
    std::set<std::filesystem::path> const path_set(paths.begin(), paths.end());
    EXPECT_TRUE(path_set.count(matching_file_a));
    EXPECT_TRUE(path_set.count(matching_file_b));
}

/**
 * @brief Test wildcard in directory component of path
 */
TEST_F(DependenciesTest, WildcardInDirectoryComponent)
{
    auto const matching_file_a = temporary_directory.create_file("subdirectory_first/target.dat", "1");
    auto const matching_file_b = temporary_directory.create_file("subdirectory_second/target.dat", "2");
    temporary_directory.create_file("other/target.dat", "3");

    Dependencies dependencies(temporary_directory.path() / "subdirectory_*/target.dat");
    auto const paths = dependencies.paths();

    ASSERT_EQ(paths.size(), 2u);
    std::set<std::filesystem::path> const path_set(paths.begin(), paths.end());
    EXPECT_TRUE(path_set.count(matching_file_a));
    EXPECT_TRUE(path_set.count(matching_file_b));
}

/**
 * @brief Test recursive wildcard traversal through nested directories
 */
TEST_F(DependenciesTest, WildcardNestedRecursiveTraversal)
{
    auto const matching_file_a = temporary_directory.create_file("level1/level2/level3/data.json", "{}");
    auto const matching_file_b = temporary_directory.create_file("level1/other/data.json", "{}");
    temporary_directory.create_file("level1/level2/other.txt", "txt");

    Dependencies dependencies(temporary_directory.path() / "level1/*.json");
    auto const paths = dependencies.paths();

    ASSERT_EQ(paths.size(), 2u);
    std::set<std::filesystem::path> const path_set(paths.begin(), paths.end());
    EXPECT_TRUE(path_set.count(matching_file_a));
    EXPECT_TRUE(path_set.count(matching_file_b));
}

/**
 * @brief Test that wildcard matching ignores subdirectories that match the pattern
 */
TEST_F(DependenciesTest, WildcardIgnoresSubdirectories)
{
    temporary_directory.create_directory("matching_directory");
    auto const file = temporary_directory.create_file("matching_file", "regular");

    Dependencies dependencies(temporary_directory.path() / "matching_*");
    auto const paths = dependencies.paths();

    ASSERT_EQ(paths.size(), 1u);
    EXPECT_EQ(paths[0], file);
}

/**
 * @brief Test wildcard pattern matching filenames with multiple dots
 */
TEST_F(DependenciesTest, WildcardWithDotsInFilename)
{
    auto const matching_file_a = temporary_directory.create_file("application.bundle.min.js", "js");
    auto const matching_file_b = temporary_directory.create_file("application.test.min.js", "js");
    temporary_directory.create_file("application.min.css", "css");

    Dependencies dependencies(temporary_directory.path() / "application.*.min.js");
    auto const paths = dependencies.paths();

    ASSERT_EQ(paths.size(), 2u);
    std::set<std::filesystem::path> const path_set(paths.begin(), paths.end());
    EXPECT_TRUE(path_set.count(matching_file_a));
    EXPECT_TRUE(path_set.count(matching_file_b));
}

/**
 * @brief Test wildcard pattern that matches no files returns empty list
 */
TEST_F(DependenciesTest, WildcardNoMatchesReturnsEmpty)
{
    temporary_directory.create_file("existing_file.txt", "content");

    Dependencies dependencies(temporary_directory.path() / "*.nonexistent");
    auto const paths = dependencies.paths();

    EXPECT_TRUE(paths.empty());
}

/**
 * @brief Test that non-existent wildcard starting directory throws runtime error
 */
TEST_F(DependenciesTest, NonExistentStartingDirectoryThrows)
{
    auto const non_existent_directory = temporary_directory.path() / "does_not_exist" / "*.txt";
    Dependencies dependencies(non_existent_directory);

    EXPECT_THROW(
        {
            try
            {
                dependencies.paths();
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_STREQ(error.what(), "Failed to load dependency path");
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test that non-existent exact path throws runtime error
 */
TEST_F(DependenciesTest, NonExistentExactPathThrows)
{
    auto const non_existent_file = temporary_directory.path() / "missing_file.txt";
    Dependencies dependencies(non_existent_file);

    EXPECT_THROW(
        {
            try
            {
                dependencies.paths();
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_STREQ(error.what(), "Failed to load dependency path");
                throw;
            }
        },
        std::runtime_error);
}
