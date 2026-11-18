#include <gtest/gtest.h>
#include "guis.hpp"
#include "test_helpers.hpp"
#include <sstream>

using namespace sss::guis;

/**
 * @brief Test fixture for the top-level Guis class
 */
class GuisTest : public ::testing::Test
{
protected:
    TemporaryDirectory temporary_directory{"guis_class_test"};
    std::filesystem::path output_directory;

    /**
     * @brief Set up test environment
     */
    void SetUp() override
    {
        output_directory = temporary_directory.path() / "output";
    }
};

/**
 * @brief Test end-to-end generation using Guis class interface
 */
TEST_F(GuisTest, EndToEndGenerationViaGuisClass)
{
    temporary_directory.create_file("widget.yaml", R"(
main:
  type: main_window
  title: "End To End GUI"
)");
    temporary_directory.create_file("theme.css", "body { margin: 0; }");

    auto const configuration_file = temporary_directory.create_file("gui_configuration.yaml", R"(
project: "TestProject"
guis:
  - name: "MainWindow"
    config: "widget.yaml"
    stylesheet: "theme.css"
)");

    Guis generator(configuration_file, output_directory);
    EXPECT_NO_THROW(generator.generate(true, false));

    EXPECT_TRUE(std::filesystem::exists(output_directory / "MainWindow.html"));
    EXPECT_TRUE(std::filesystem::exists(output_directory / "theme.css"));
}

/**
 * @brief Test end-to-end generation with debug stream output
 */
TEST_F(GuisTest, EndToEndWithDebugStream)
{
    temporary_directory.create_file("widget.yaml", R"(
main:
  type: main_window
)");
    temporary_directory.create_file("theme.css", "body {}");

    auto const configuration_file = temporary_directory.create_file("gui_configuration.yaml", R"(
project: "DebugTest"
guis:
  - name: "DebugWindow"
    config: "widget.yaml"
    stylesheet: "theme.css"
)");

    std::stringstream debug_stream;
    Guis generator(configuration_file, output_directory);
    EXPECT_NO_THROW(generator.generate(true, false, &debug_stream));

    std::string const debug_output = debug_stream.str();
    EXPECT_FALSE(debug_output.empty());
    EXPECT_NE(debug_output.find("DebugWindow"), std::string::npos);
}
