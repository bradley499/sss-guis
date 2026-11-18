#include <gtest/gtest.h>
#include "generation.hpp"
#include "test_helpers.hpp"
#include <nlohmann/json.hpp>
#include <regex>

using namespace sss::guis;


/**
 * @brief Test fixture for the Generation class
 */
class GenerationTest : public ::testing::Test
{
protected:
    TemporaryDirectory temporary_directory{"generation_test"};
    std::filesystem::path output_directory;

    /**
     * @brief Set up test environment
     */
    void SetUp() override
    {
        output_directory = temporary_directory.path() / "output";
    }

    /**
     * @brief Create a minimal valid widget configuration file
     * @param name Name of the configuration file to create
     * @return Path to the created configuration file
     */
    std::filesystem::path create_minimal_widget_config(std::string const &name = "main_config.yaml")
    {
        return temporary_directory.create_file(name, R"(
main:
  type: root
  title: "Minimal Window"
)");
    }

    /**
     * @brief Create a minimal valid stylesheet file
     * @param name Name of the stylesheet file to create
     * @return Path to the created stylesheet file
     */
    std::filesystem::path create_minimal_stylesheet(std::string const &name = "style.css")
    {
        return temporary_directory.create_file(name, "body { background: #000; }");
    }
};

/**
 * @brief Test Missing Guis Key Throws
 */
TEST_F(GenerationTest, MissingGuisKeyThrows)
{
    auto const configuration_file = temporary_directory.create_file("invalid.yaml", R"(
project: "Test"
)");

    EXPECT_THROW(
        {
            try
            {
                Generation generator(configuration_file, output_directory);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("Expected list of `guis` in configuration"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Gui Entry Not Map Throws
 */
TEST_F(GenerationTest, GuiEntryNotMapThrows)
{
    auto const configuration_file = temporary_directory.create_file("invalid.yaml", R"(
guis:
  - "not a map"
)");

    EXPECT_THROW(
        {
            try
            {
                Generation generator(configuration_file, output_directory);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("Expected `gui` to hold a defined configuration"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Missing Required Fields Throws
 */
TEST_F(GenerationTest, MissingRequiredFieldsThrows)
{
    auto const widget_configuration = create_minimal_widget_config();
    auto const stylesheet = create_minimal_stylesheet();

    // Missing name
    auto const configuration_file_without_name = temporary_directory.create_file("no_name.yaml", R"(
guis:
  - config: ")" + widget_configuration.string() + R"("
    stylesheet: ")" + stylesheet.string() + R"("
)");
    EXPECT_THROW(Generation(configuration_file_without_name, output_directory), std::runtime_error);

    // Missing config
    auto const configuration_file_without_config = temporary_directory.create_file("no_config.yaml", R"(
guis:
  - name: "App"
    stylesheet: ")" + stylesheet.string() + R"("
)");
    EXPECT_THROW(Generation(configuration_file_without_name, output_directory), std::runtime_error);

    // Missing stylesheet
    auto const configuration_file_without_stylesheet = temporary_directory.create_file("no_style.yaml", R"(
guis:
  - name: "App"
    config: ")" + widget_configuration.string() + R"("
)");
    EXPECT_THROW(Generation(configuration_file_without_name, output_directory), std::runtime_error);
}

/**
 * @brief Test Empty Or Forbidden Name Throws
 */
TEST_F(GenerationTest, EmptyOrForbiddenNameThrows)
{
    auto const widget_configuration = create_minimal_widget_config();
    auto const stylesheet = create_minimal_stylesheet();

    auto const configuration_file_with_empty_name = temporary_directory.create_file("empty_name.yaml", R"(
guis:
  - name: "   "
    config: ")" + widget_configuration.string() + R"("
    stylesheet: ")" + stylesheet.string() + R"("
)");
    EXPECT_THROW(
        {
            try
            {
                Generation generator(configuration_file_with_empty_name, output_directory);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("A name must not be empty"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);

    auto const configuration_file_with_forbidden_name = temporary_directory.create_file("forbidden_name.yaml", R"(
guis:
  - name: ":*?\"<>|"
    config: ")" + widget_configuration.string() + R"("
    stylesheet: ")" + stylesheet.string() + R"("
)");
    EXPECT_THROW(
        {
            try
            {
                Generation generator(configuration_file_with_forbidden_name, output_directory);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("invalid filesystem characters"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Missing Source Config Files Throws
 */
TEST_F(GenerationTest, MissingSourceConfigFilesThrows)
{
    auto const stylesheet = create_minimal_stylesheet();
    auto configuration = temporary_directory.create_file("missing_configuration.yaml", R"(
guis:
  - name: "App"
    config: "non_existent_config.yaml"
    stylesheet: ")" + stylesheet.string() + R"("
)");

    EXPECT_THROW(
        {
            try
            {
                Generation generator(configuration, output_directory);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("Unable to find a source config file"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Missing Stylesheet File Throws
 */
TEST_F(GenerationTest, MissingStylesheetFileThrows)
{
    auto const widget_configuration = create_minimal_widget_config();
    auto configuration = temporary_directory.create_file("missing_style.yaml", R"(
guis:
  - name: "App"
    config: ")" + widget_configuration.string() + R"("
    stylesheet: "non_existent_style.css"
)");

    EXPECT_THROW(
        {
            try
            {
                Generation generator(configuration, output_directory);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("Unable to find the stylesheet"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Output Directory Is File Throws
 */
TEST_F(GenerationTest, OutputDirectoryIsFileThrows)
{
    auto const widget_configuration = create_minimal_widget_config();
    auto const stylesheet = create_minimal_stylesheet();
    auto existing_file = temporary_directory.create_file("regular_file", "i am a file");

    auto configuration = temporary_directory.create_file("config.yaml", R"(
guis:
  - name: "App"
    config: ")" + widget_configuration.string() + R"("
    stylesheet: ")" + stylesheet.string() + R"("
)");

    EXPECT_THROW(
        {
            try
            {
                Generation generator(configuration, existing_file);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("already a file located as the output directory"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Html Output Already Exists Throws
 */
TEST_F(GenerationTest, HtmlOutputAlreadyExistsThrows)
{
    auto const widget_configuration = create_minimal_widget_config();
    auto const stylesheet = create_minimal_stylesheet();
    temporary_directory.create_file("output/App.html", "pre-existing HTML");

    auto configuration = temporary_directory.create_file("config.yaml", R"(
guis:
  - name: "App"
    config: ")" + widget_configuration.string() + R"("
    stylesheet: ")" + stylesheet.string() + R"("
)");

    EXPECT_THROW(
        {
            try
            {
                Generation generator(configuration, output_directory);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("as a file already exists with that name"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Conflicting Gui Names Throws
 */
TEST_F(GenerationTest, ConflictingGuiNamesThrows)
{
    auto const widget_configuration = create_minimal_widget_config();
    auto const stylesheet = create_minimal_stylesheet();

    // 1. Same exact name
    auto const configuration_file_with_same_names = temporary_directory.create_file("same_name.yaml", R"(
guis:
  - name: "Dashboard"
    config: ")" + widget_configuration.string() + R"("
    stylesheet: ")" + stylesheet.string() + R"("
  - name: "Dashboard"
    config: ")" + widget_configuration.string() + R"("
    stylesheet: ")" + stylesheet.string() + R"("
)");
    Generation generator_first(configuration_file_with_same_names, output_directory);
    EXPECT_THROW(
        {
            try
            {
                generator_first.build_all(true);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("Multiple GUIs share the same name"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);

    // 2. Different names sanitizing to same HTML file
    auto const configuration_file_with_colliding_paths = temporary_directory.create_file("colliding_name.yaml", R"(
guis:
  - name: "Dash:board"
    config: ")" + widget_configuration.string() + R"("
    stylesheet: ")" + stylesheet.string() + R"("
  - name: "Dash?board"
    config: ")" + widget_configuration.string() + R"("
    stylesheet: ")" + stylesheet.string() + R"("
)");
    Generation generator_second(configuration_file_with_colliding_paths, output_directory);
    EXPECT_THROW(
        {
            try
            {
                generator_second.build_all(true);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("will both yield the same output filename"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Flattened Dependencies Conflict Throws
 */
TEST_F(GenerationTest, FlattenedDependenciesConflictThrows)
{
    auto const widget_configuration = create_minimal_widget_config();
    auto const stylesheet = create_minimal_stylesheet();
    temporary_directory.create_file("dir1/common.txt", "1");
    temporary_directory.create_file("dir2/common.txt", "2");

    auto configuration = temporary_directory.create_file("flatten_conflict.yaml", R"(
guis:
  - name: "App"
    config: ")" + widget_configuration.string() + R"("
    stylesheet: ")" + stylesheet.string() + R"("
    dependencies:
      - "dir1/common.txt"
      - "dir2/common.txt"
)");

    Generation generator(configuration, output_directory);
    EXPECT_THROW(
        {
            try
            {
                generator.build_all(true, true); // flatten = true
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("between flattened dependencies"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Successful Full Generation
 */
TEST_F(GenerationTest, SuccessfulFullGeneration)
{
    auto const widget_configuration = create_minimal_widget_config("widget.yaml");
    auto const stylesheet = temporary_directory.create_file("theme.css", "h1 { color: red; }");
    auto module_js = temporary_directory.create_file("modules/custom_mod.js", "console.log('loaded');");
    auto asset_file = temporary_directory.create_file("assets/image.png", "fake_png");

    auto const main_configuration = temporary_directory.create_file("main_gui.yaml", R"(
project: "TestSystem"
project_delimiter: " :: "
guis:
  - name: "My Dashboard"
    config: "widget.yaml"
    stylesheet: "theme.css"
    modules:
      - "modules/custom_mod.js"
    dependencies:
      - "assets/*.png"
    debug: false
)");

    Generation generator(main_configuration, output_directory);
    EXPECT_NO_THROW(generator.build_all(true, false));

    // Verify HTML file was generated
    std::filesystem::path html_path = output_directory / "My Dashboard.html";
    ASSERT_TRUE(std::filesystem::exists(html_path));

    std::string html_content = temporary_directory.read_file("output/My Dashboard.html");
    EXPECT_NE(html_content.find("<!DOCTYPE html>"), std::string::npos);
    EXPECT_NE(html_content.find("<title>TestSystem</title>"), std::string::npos);
    EXPECT_NE(html_content.find("const gui="), std::string::npos);

    // Parse the embedded gui JSON object from HTML
    std::regex gui_regex(R"(const gui=(\{.*?\});)");
    std::smatch match;
    ASSERT_TRUE(std::regex_search(html_content, match, gui_regex));
    std::string gui_json_str = match[1].str();

    auto gui_json = nlohmann::json::parse(gui_json_str);
    EXPECT_EQ(gui_json["project"].get<std::string>(), "TestSystem");
    EXPECT_EQ(gui_json["name"].get<std::string>(), "My Dashboard :: TestSystem");
    EXPECT_EQ(gui_json["stylesheet"].get<std::string>(), "theme.css");
    ASSERT_EQ(gui_json["modules"].size(), 1u);
    EXPECT_EQ(gui_json["modules"][0].get<std::string>(), "modules/custom_mod.js");

    std::string structure_rel = gui_json["structure"].get<std::string>();
    std::filesystem::path structure_path = output_directory / structure_rel;
    EXPECT_TRUE(std::filesystem::exists(structure_path));

    auto structure_json = nlohmann::json::parse(temporary_directory.read_file("output/" + structure_rel));
    EXPECT_TRUE(structure_json.contains("widgets"));
    EXPECT_TRUE(structure_json.contains("types"));

    // Verify module and stylesheet were copied
    EXPECT_TRUE(std::filesystem::exists(output_directory / "theme.css"));
    EXPECT_TRUE(std::filesystem::exists(output_directory / "modules/custom_mod.js"));
    EXPECT_TRUE(std::filesystem::exists(output_directory / "assets/image.png"));

    // Verify bundled JS file was written to output dir
    bool found_js_bundle = false;
    for (auto const &entry : std::filesystem::directory_iterator(output_directory))
    {
        if (entry.path().extension() == ".js" && entry.path().filename() != "custom_mod.js")
        {
            found_js_bundle = true;
            EXPECT_GT(std::filesystem::file_size(entry.path()), 0u);
        }
    }
    EXPECT_TRUE(found_js_bundle);
}

/**
 * @brief Test Flatten Dependency References
 */
TEST_F(GenerationTest, FlattenDependencyReferences)
{
    auto const widget_configuration = create_minimal_widget_config("widget.yaml");
    temporary_directory.create_file("styles/nested/theme.css", "body {}");
    temporary_directory.create_file("scripts/sub/mod.js", "mod");
    temporary_directory.create_file("data/deep/asset.txt", "data");

    auto const main_configuration = temporary_directory.create_file("main_gui.yaml", R"(
guis:
  - name: "FlattenApp"
    config: "widget.yaml"
    stylesheet: "styles/nested/theme.css"
    modules:
      - "scripts/sub/mod.js"
    dependencies:
      - "data/deep/asset.txt"
)");

    Generation generator(main_configuration, output_directory);
    EXPECT_NO_THROW(generator.build_all(true, true)); // flatten = true

    // Verify files copied to root of output_directory
    EXPECT_TRUE(std::filesystem::exists(output_directory / "theme.css"));
    EXPECT_TRUE(std::filesystem::exists(output_directory / "mod.js"));
    EXPECT_TRUE(std::filesystem::exists(output_directory / "asset.txt"));
    EXPECT_FALSE(std::filesystem::exists(output_directory / "styles/nested/theme.css"));
}

/**
 * @brief Test Nested Gui Name Directory Depth
 */
TEST_F(GenerationTest, NestedGuiNameDirectoryDepth)
{
    auto const widget_configuration = create_minimal_widget_config("widget.yaml");
    auto const stylesheet = create_minimal_stylesheet("style.css");

    auto const main_configuration = temporary_directory.create_file("main_gui.yaml", R"(
guis:
  - name: "admin/dashboard/overview"
    config: "widget.yaml"
    stylesheet: "style.css"
)");

    Generation generator(main_configuration, output_directory);
    EXPECT_NO_THROW(generator.build_all(true, false));

    // HTML should be located at admin/dashboard/overview.html
    EXPECT_TRUE(std::filesystem::exists(output_directory / "admin/dashboard/overview.html"));

    std::string html_content = temporary_directory.read_file("output/admin/dashboard/overview.html");
    std::regex gui_regex(R"(const gui=(\{.*?\});)");
    std::smatch match;
    ASSERT_TRUE(std::regex_search(html_content, match, gui_regex));
    auto gui_json = nlohmann::json::parse(match[1].str());

    // Relative adjustment for depth 2 ("admin/dashboard") should be "../../"
    EXPECT_EQ(gui_json["stylesheet"].get<std::string>(), "../../style.css");
    std::string struct_file = gui_json["structure"].get<std::string>();
    EXPECT_EQ(struct_file.rfind("../../", 0), 0u);
}

/**
 * @brief Test Dynamic File Tag Registration During Build
 */
TEST_F(GenerationTest, DynamicFileTagRegistrationDuringBuild)
{
    auto asset = temporary_directory.create_file("untracked_image.png", "img_binary");
    auto const widget_configuration = temporary_directory.create_file("widget.yaml", R"(
main:
  type: img
  source: !file untracked_image.png
)");
    auto const stylesheet = create_minimal_stylesheet();

    auto const main_configuration = temporary_directory.create_file("main_gui.yaml", R"(
guis:
  - name: "App"
    config: "widget.yaml"
    stylesheet: "style.css"
)");

    Generation generator(main_configuration, output_directory);
    EXPECT_NO_THROW(generator.build_all(true, false));

    // The untracked file should be copied to the output directory with a generated filename
    bool copied_image = false;
    for (auto const &entry : std::filesystem::directory_iterator(output_directory))
    {
        if (entry.path().extension() == ".png")
        {
            copied_image = true;
            EXPECT_EQ(std::filesystem::file_size(entry.path()), std::filesystem::file_size(asset));
        }
    }
    EXPECT_TRUE(copied_image);
}

/**
 * @brief Test Parallel Multi Gui Generation
 */
TEST_F(GenerationTest, ParallelMultiGuiGeneration)
{
    auto const stylesheet = create_minimal_stylesheet();
    std::string config_content = "guis:\n";

    for (int i = 0; i < 5; ++i)
    {
        std::string configuration_name = "widget_" + std::to_string(i) + ".yaml";
        temporary_directory.create_file(configuration_name, "main:\n  type: root\n  id: " + std::to_string(i));
        config_content += "  - name: \"App_" + std::to_string(i) + "\"\n";
        config_content += "    config: \"" + configuration_name + "\"\n";
        config_content += "    stylesheet: \"" + stylesheet.string() + "\"\n";
    }

    auto const main_configuration = temporary_directory.create_file("multi_gui.yaml", config_content);
    Generation generator(main_configuration, output_directory);
    EXPECT_NO_THROW(generator.build_all(true, false));

    for (int i = 0; i < 5; ++i)
        EXPECT_TRUE(std::filesystem::exists(output_directory / ("App_" + std::to_string(i) + ".html")));
}

/**
 * @brief Test Single Config File With Unique Json Verification Numeric
 */
TEST_F(GenerationTest, SingleConfigFileWithUniqueJsonVerification_Numeric)
{
    auto const stylesheet = create_minimal_stylesheet("theme.css");

    // Single widget configuration file containing:
    // - main widget with child reference to panel
    // - panel widget with child reference to button
    // - button widget
    // - orphan widget that must be pruned
    auto const widget_configuration = temporary_directory.create_file("widgets.yaml", R"(
main:
  type: root_view
  title: "Single Config App"
  panel:
    object: content_panel
content_panel:
  type: panel_box
  orientation: "vertical"
  action_btn:
    object: submit_btn
submit_btn:
  type: button_control
  label: "Submit"
orphan_widget:
  type: unused_orphan_type
  orphan_prop: "should_not_exist_in_output"
)");

    // Top-level GUI config with defaults for button_control
    auto const gui_configuration = temporary_directory.create_file("app.yaml", R"(
project: "SingleConfigProject"
guis:
  - name: "SingleApp"
    config: "widgets.yaml"
    stylesheet: "theme.css"
    debug: false
    defaults:
      button_control:
        variant: "primary"
        disabled: false
)");

    Generation generator(gui_configuration, output_directory);
    EXPECT_NO_THROW(generator.build_all(true, false));

    // 1. Verify HTML generated
    std::filesystem::path html_path = output_directory / "SingleApp.html";
    ASSERT_TRUE(std::filesystem::exists(html_path));

    std::string html_content = temporary_directory.read_file("output/SingleApp.html");
    auto gui_meta = extract_gui_json_from_html(html_content);

    EXPECT_EQ(gui_meta["project"].get<std::string>(), "SingleConfigProject");
    EXPECT_EQ(gui_meta["name"].get<std::string>(), "SingleApp | SingleConfigProject");
    EXPECT_EQ(gui_meta["stylesheet"].get<std::string>(), "theme.css");

    // 2. Extract uniquely named JSON file path from HTML metadata
    std::string structure_rel = gui_meta["structure"].get<std::string>();
    std::filesystem::path structure_file = output_directory / structure_rel;

    ASSERT_TRUE(std::filesystem::exists(structure_file));
    EXPECT_EQ(structure_file.extension(), ".json");

    // 3. Parse uniquely named JSON file and verify exact contents
    auto structure_json = nlohmann::json::parse(temporary_directory.read_file("output/" + structure_rel));

    // Verify types: "unused_orphan_type" must have been pruned!
    ASSERT_TRUE(structure_json.contains("types"));
    auto const &types = structure_json["types"];
    ASSERT_EQ(types.size(), 3u);
    // Encountered order: root_view (0), panel_box (1), button_control (2)
    EXPECT_EQ(types[0].get<std::string>(), "root_view");
    EXPECT_EQ(types[1].get<std::string>(), "panel_box");
    EXPECT_EQ(types[2].get<std::string>(), "button_control");

    // In numeric mode, widgets are sorted alphabetically in std::map:
    // Index 0: "content_panel" (type 1)
    // Index 1: "main" (type 0)
    // Index 2: "submit_btn" (type 2)
    EXPECT_TRUE(structure_json.contains("main"));
    EXPECT_EQ(structure_json["main"].get<int>(), 1); // "main" is at index 1

    ASSERT_TRUE(structure_json.contains("widgets"));
    auto const &widgets = structure_json["widgets"];
    ASSERT_EQ(widgets.size(), 3u); // orphan_widget was pruned!

    // Widget 0: content_panel
    EXPECT_EQ(widgets[0][0].get<int>(), 1); // type panel_box
    EXPECT_EQ(widgets[0][1]["orientation"].get<std::string>(), "vertical");
    EXPECT_EQ(widgets[0][1]["action_btn"]["object"].get<int>(), 2); // points to submit_btn (idx 2)

    // Widget 1: main
    EXPECT_EQ(widgets[1][0].get<int>(), 0); // type root_view
    EXPECT_EQ(widgets[1][1]["title"].get<std::string>(), "Single Config App");
    EXPECT_EQ(widgets[1][1]["panel"]["object"].get<int>(), 0); // points to content_panel (idx 0)

    // Widget 2: submit_btn (with inherited defaults)
    EXPECT_EQ(widgets[2][0].get<int>(), 2); // type button_control
    EXPECT_EQ(widgets[2][1]["label"].get<std::string>(), "Submit");
    EXPECT_EQ(widgets[2][1]["variant"].get<std::string>(), "primary");
    EXPECT_EQ(widgets[2][1]["disabled"].get<bool>(), false);
}

/**
 * @brief Test Single Config File With Unique Json Verification Debug Non Numeric
 */
TEST_F(GenerationTest, SingleConfigFileWithUniqueJsonVerification_DebugNonNumeric)
{
    auto const stylesheet = create_minimal_stylesheet("style.css");

    auto const widget_configuration = temporary_directory.create_file("widgets.yaml", R"(
main:
  type: debug_window
  header:
    object: header_widget
header_widget:
  type: header_box
  text: "Welcome"
)");

    auto const gui_configuration = temporary_directory.create_file("app.yaml", R"(
guis:
  - name: "DebugApp"
    config: "widgets.yaml"
    stylesheet: "style.css"
    debug: true
)");

    Generation generator(gui_configuration, output_directory);
    EXPECT_NO_THROW(generator.build_all(true, false));

    std::string html_content = temporary_directory.read_file("output/DebugApp.html");
    auto gui_meta = extract_gui_json_from_html(html_content);
    std::string structure_rel = gui_meta["structure"].get<std::string>();

    auto structure_json = nlohmann::json::parse(temporary_directory.read_file("output/" + structure_rel));

    // In debug mode:
    // main is string "main"
    EXPECT_EQ(structure_json["main"].get<std::string>(), "main");

    // widgets is a map/object with widget names as keys
    ASSERT_TRUE(structure_json["widgets"].is_object());
    ASSERT_EQ(structure_json["widgets"].size(), 2u);
    EXPECT_TRUE(structure_json["widgets"].contains("main"));
    EXPECT_TRUE(structure_json["widgets"].contains("header_widget"));

    // Object references remain strings
    EXPECT_EQ(structure_json["widgets"]["main"][1]["header"]["object"].get<std::string>(), "header_widget");
    EXPECT_EQ(structure_json["widgets"]["header_widget"][1]["text"].get<std::string>(), "Welcome");
}

/**
 * @brief Test Multiple Config Files Chained Dependencies Unique Json Verification
 */
TEST_F(GenerationTest, MultipleConfigFilesChainedDependencies_UniqueJsonVerification)
{
    auto const stylesheet = create_minimal_stylesheet("app.css");

    // Config file 3: buttons.yaml (deepest dependency)
    temporary_directory.create_file("components/buttons.yaml", R"(
nav_btn:
  type: button
  caption: "Home"
unused_component_btn:
  type: button
  caption: "Never Used"
)");

    // Config file 2: nav.yaml (depends on buttons.yaml)
    temporary_directory.create_file("components/nav.yaml", R"(
dependencies:
  - buttons.yaml
nav_bar:
  type: navigation
  home_btn:
    object: nav_btn
)");

    // Config file 1: main_widgets.yaml (root widget config, depends on nav.yaml)
    temporary_directory.create_file("main_widgets.yaml", R"(
dependencies:
  - components/nav.yaml
main:
  type: main_layout
  nav:
    object: nav_bar
)");

    // Top-level GUI config referencing main_widgets.yaml
    auto const gui_configuration = temporary_directory.create_file("gui.yaml", R"(
project: "MultiConfigProject"
guis:
  - name: "ChainedApp"
    config: "main_widgets.yaml"
    stylesheet: "app.css"
)");

    Generation generator(gui_configuration, output_directory);
    EXPECT_NO_THROW(generator.build_all(true, false));

    // Verify HTML and read uniquely named structure JSON
    std::string html_content = temporary_directory.read_file("output/ChainedApp.html");
    auto structure_json = read_structure_json_from_html(output_directory, html_content);

    // Verify widgets loaded across all 3 files are present
    // Widgets alphabetically:
    // Index 0: "main" (type 0: main_layout)
    // Index 1: "nav_bar" (type 1: navigation)
    // Index 2: "nav_btn" (type 2: button)
    // "unused_component_btn" in buttons.yaml must be pruned!
    ASSERT_EQ(structure_json["widgets"].size(), 3u);

    EXPECT_EQ(structure_json["main"].get<int>(), 0);

    // main (idx 0) references nav_bar (idx 1)
    EXPECT_EQ(structure_json["widgets"][0][1]["nav"]["object"].get<int>(), 1);

    // nav_bar (idx 1) references nav_btn (idx 2)
    EXPECT_EQ(structure_json["widgets"][1][1]["home_btn"]["object"].get<int>(), 2);

    // nav_btn (idx 2) has caption "Home"
    EXPECT_EQ(structure_json["widgets"][2][1]["caption"].get<std::string>(), "Home");
}

/**
 * @brief Test Multiple Gui Configs In Single File Unique Json Verification
 */
TEST_F(GenerationTest, MultipleGuiConfigsInSingleFile_UniqueJsonVerification)
{
    auto const stylesheet = create_minimal_stylesheet("common.css");

    // Widget config for GUI 1: Dashboard
    temporary_directory.create_file("dashboard_widgets.yaml", R"(
main:
  type: dashboard_view
  kpi: "Sales"
)");

    // Widget config for GUI 2: Settings
    temporary_directory.create_file("settings_widgets.yaml", R"(
main:
  type: settings_view
  theme_mode: "dark"
)");

    // Single GUI configuration file with 2 GUIs
    auto const gui_configuration = temporary_directory.create_file("multi_gui_config.yaml", R"(
project: "EnterprisePortal"
guis:
  - name: "Dashboard"
    config: "dashboard_widgets.yaml"
    stylesheet: "common.css"
  - name: "Settings"
    config: "settings_widgets.yaml"
    stylesheet: "common.css"
)");

    Generation generator(gui_configuration, output_directory);
    EXPECT_NO_THROW(generator.build_all(true, false));

    // Read HTML for both GUIs
    std::string dash_html = temporary_directory.read_file("output/Dashboard.html");
    std::string settings_html = temporary_directory.read_file("output/Settings.html");

    auto dash_meta = extract_gui_json_from_html(dash_html);
    auto settings_meta = extract_gui_json_from_html(settings_html);

    std::string dash_struct_file = dash_meta["structure"].get<std::string>();
    std::string settings_struct_file = settings_meta["structure"].get<std::string>();

    // Verify both structure JSON files have UNIQUE filenames!
    EXPECT_NE(dash_struct_file, settings_struct_file);
    EXPECT_TRUE(std::filesystem::exists(output_directory / dash_struct_file));
    EXPECT_TRUE(std::filesystem::exists(output_directory / settings_struct_file));

    // Verify Dashboard JSON matches dashboard configuration
    auto dash_json = nlohmann::json::parse(temporary_directory.read_file("output/" + dash_struct_file));
    EXPECT_EQ(dash_json["types"][0].get<std::string>(), "dashboard_view");
    EXPECT_EQ(dash_json["widgets"][0][1]["kpi"].get<std::string>(), "Sales");

    // Verify Settings JSON matches settings configuration
    auto settings_json = nlohmann::json::parse(temporary_directory.read_file("output/" + settings_struct_file));
    EXPECT_EQ(settings_json["types"][0].get<std::string>(), "settings_view");
    EXPECT_EQ(settings_json["widgets"][0][1]["theme_mode"].get<std::string>(), "dark");
}

