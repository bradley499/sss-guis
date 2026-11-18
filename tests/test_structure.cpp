#include <gtest/gtest.h>
#include "structure.hpp"
#include "test_helpers.hpp"
#include <nlohmann/json.hpp>
#include <sstream>

using namespace sss::guis;


/**
 * @brief Test fixture for the Structure class
 */
class StructureTest : public ::testing::Test
{
protected:
    TemporaryDirectory temporary_directory{"structure_test"};

    PathRegisterDependency register_dependency_callback = [](std::filesystem::path const &path) -> std::string
    {
        return "registered/" + path.filename().string();
    };
};

/**
 * @brief Test Basic Single Widget Numeric Build
 */
TEST_F(StructureTest, BasicSingleWidgetNumericBuild)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: root_window
  title: "My Application"
  width: 800
  height: 600
  visible: true
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);
    std::string const structure_json = structure.build(register_dependency_callback, true);

    auto const parsed = nlohmann::json::parse(structure_json);
    EXPECT_TRUE(parsed.contains("main"));
    EXPECT_TRUE(parsed["main"].is_number_integer());
    EXPECT_EQ(parsed["main"].get<int>(), 0);

    EXPECT_TRUE(parsed.contains("types"));
    EXPECT_EQ(parsed["types"].size(), 1u);
    EXPECT_EQ(parsed["types"][0].get<std::string>(), "root_window");

    EXPECT_TRUE(parsed.contains("widgets"));
    EXPECT_TRUE(parsed["widgets"].is_array());
    ASSERT_EQ(parsed["widgets"].size(), 1u);

    // Each widget entry is [type_id, contents]
    auto const &widget = parsed["widgets"][0];
    EXPECT_EQ(widget[0].get<int>(), 0);
    EXPECT_EQ(widget[1]["title"].get<std::string>(), "My Application");
    EXPECT_EQ(widget[1]["width"].get<int>(), 800);
    EXPECT_EQ(widget[1]["height"].get<int>(), 600);
    EXPECT_EQ(widget[1]["visible"].get<bool>(), true);
}

/**
 * @brief Test Basic Single Widget Non Numeric Build
 */
TEST_F(StructureTest, BasicSingleWidgetNonNumericBuild)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: root_window
  title: "Debug View"
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);
    std::string const structure_json = structure.build(register_dependency_callback, false);

    auto const parsed = nlohmann::json::parse(structure_json);
    EXPECT_TRUE(parsed.contains("main"));
    EXPECT_EQ(parsed["main"].get<std::string>(), "main");

    EXPECT_TRUE(parsed.contains("widgets"));
    EXPECT_TRUE(parsed["widgets"].is_object());
    EXPECT_TRUE(parsed["widgets"].contains("main"));

    auto const &main_widget = parsed["widgets"]["main"];
    EXPECT_EQ(main_widget[0].get<int>(), 0);
    EXPECT_EQ(main_widget[1]["title"].get<std::string>(), "Debug View");
}

/**
 * @brief Test Missing Main Widget Throws
 */
TEST_F(StructureTest, MissingMainWidgetThrows)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
other_widget:
  type: container
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);

    EXPECT_THROW(
        {
            try
            {
                structure.build(register_dependency_callback);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_STREQ(error.what(), "No `main` widget was found!");
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Widget Name Case Insensitivity
 */
TEST_F(StructureTest, WidgetNameCaseInsensitivity)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
MAIN:
  type: container
  child:
    object: CHILD_BOX
CHILD_BOX:
  type: box
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);
    std::string const structure_json = structure.build(register_dependency_callback, false);

    auto const parsed = nlohmann::json::parse(structure_json);
    EXPECT_EQ(parsed["main"].get<std::string>(), "main");
    EXPECT_TRUE(parsed["widgets"].contains("main"));
    EXPECT_TRUE(parsed["widgets"].contains("child_box"));
}

/**
 * @brief Test Widget Name With Whitespace Throws
 */
TEST_F(StructureTest, WidgetNameWithWhitespaceThrows)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
"my widget":
  type: container
)");

    Structure structure("test_struct");
    EXPECT_THROW(
        {
            try
            {
                structure.parse_file(configuration_file);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("due to whitespace within its name"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Widget Named Null Throws
 */
TEST_F(StructureTest, WidgetNamedNullThrows)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
"null":
  type: container
)");

    Structure structure("test_struct");
    EXPECT_THROW(
        {
            try
            {
                structure.parse_file(configuration_file);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("as that name is not allowed"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Widget With No Name Throws
 */
TEST_F(StructureTest, WidgetWithNoNameThrows)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
"":
  type: container
)");

    Structure structure("test_struct");
    EXPECT_THROW(
        {
            try
            {
                structure.parse_file(configuration_file);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("Failed to parse widget with no name defined"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Widget Type Allowed Characters
 */
TEST_F(StructureTest, WidgetTypeAllowedCharacters)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: my_Type.123-abc
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);
    std::string const structure_json = structure.build(register_dependency_callback);

    auto const parsed = nlohmann::json::parse(structure_json);
    EXPECT_EQ(parsed["types"][0].get<std::string>(), "my_type.123-abc");
}

/**
 * @brief Test Widget Type Invalid Characters Throws
 */
TEST_F(StructureTest, WidgetTypeInvalidCharactersThrows)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: invalid@type!
)");

    Structure structure("test_struct");
    EXPECT_THROW(
        {
            try
            {
                structure.parse_file(configuration_file);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("contains invalid characters"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Widget Missing Type Throws
 */
TEST_F(StructureTest, WidgetMissingTypeThrows)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  property: value
)");

    Structure structure("test_struct");
    EXPECT_THROW(
        {
            try
            {
                structure.parse_file(configuration_file);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("has no `type` definition"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Duplicate Widget Definitions Throws
 */
TEST_F(StructureTest, DuplicateWidgetDefinitionsThrows)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: box
main:
  type: container
)");

    Structure structure("test_struct");
    EXPECT_THROW(structure.parse_file(configuration_file), std::runtime_error);
}

/**
 * @brief Test Yaml Data Types To Json Conversion
 */
TEST_F(StructureTest, YamlDataTypesToJsonConversion)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: root
  int_val: 42
  negative_int: -17
  double_val: 3.14159
  bool_true: true
  bool_false: false
  string_val: "Hello YAML"
  string_alphanumeric: "item_123"
  list_val:
    - 1
    - "two"
    - 3.0
    - false
  nested_map:
    sub_key: "sub_value"
    sub_int: 99
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);
    std::string const structure_json = structure.build(register_dependency_callback, false);

    auto const parsed = nlohmann::json::parse(structure_json);
    auto const &props = parsed["widgets"]["main"][1];

    EXPECT_EQ(props["int_val"].get<int>(), 42);
    EXPECT_EQ(props["negative_int"].get<int>(), -17);
    EXPECT_NEAR(props["double_val"].get<double>(), 3.14159, 0.0001);
    EXPECT_EQ(props["bool_true"].get<bool>(), true);
    EXPECT_EQ(props["bool_false"].get<bool>(), false);
    EXPECT_EQ(props["string_val"].get<std::string>(), "Hello YAML");
    EXPECT_EQ(props["string_alphanumeric"].get<std::string>(), "item_123");

    EXPECT_TRUE(props["list_val"].is_array());
    EXPECT_EQ(props["list_val"][0].get<int>(), 1);
    EXPECT_EQ(props["list_val"][1].get<std::string>(), "two");
    EXPECT_NEAR(props["list_val"][2].get<double>(), 3.0, 0.0001);
    EXPECT_EQ(props["list_val"][3].get<bool>(), false);

    EXPECT_TRUE(props["nested_map"].is_object());
    EXPECT_EQ(props["nested_map"]["sub_key"].get<std::string>(), "sub_value");
    EXPECT_EQ(props["nested_map"]["sub_int"].get<int>(), 99);
}

/**
 * @brief Test Widget Defaults Inheritance And Override
 */
TEST_F(StructureTest, WidgetDefaultsInheritanceAndOverride)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: custom_btn
  label: "Submit"
  override_prop: "custom_value"
)");

    YAML::Node defaults = YAML::Load(R"(
custom_btn:
  default_prop: "default_value"
  override_prop: "default_to_be_overridden"
  count: 10
  layout:
    padding: 5
    margin: 10
)");

    Structure structure("test_struct");
    structure.populate_widget_defaults(defaults);
    structure.parse_file(configuration_file);
    std::string const structure_json = structure.build(register_dependency_callback, false);

    auto const parsed = nlohmann::json::parse(structure_json);
    auto const &props = parsed["widgets"]["main"][1];

    EXPECT_EQ(props["default_prop"].get<std::string>(), "default_value");
    EXPECT_EQ(props["override_prop"].get<std::string>(), "custom_value");
    EXPECT_EQ(props["count"].get<int>(), 10);
    EXPECT_EQ(props["label"].get<std::string>(), "Submit");
    EXPECT_EQ(props["layout"]["padding"].get<int>(), 5);
    EXPECT_EQ(props["layout"]["margin"].get<int>(), 10);
}

/**
 * @brief Test Widget Defaults Redundant Type Throws
 */
TEST_F(StructureTest, WidgetDefaultsRedundantTypeThrows)
{
    YAML::Node defaults = YAML::Load(R"(
button:
  type: button
)");

    Structure structure("test_struct");
    EXPECT_THROW(
        {
            try
            {
                structure.populate_widget_defaults(defaults);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("has a redundant `type` definition"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Widget Defaults Conflicting Type Throws
 */
TEST_F(StructureTest, WidgetDefaultsConflictingTypeThrows)
{
    YAML::Node defaults = YAML::Load(R"(
button:
  type: different_type
)");

    Structure structure("test_struct");
    EXPECT_THROW(
        {
            try
            {
                structure.populate_widget_defaults(defaults);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("conflicting yet redundant `type` definition"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Widget Defaults Structural Conflict Throws
 */
TEST_F(StructureTest, WidgetDefaultsStructuralConflictThrows)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: panel
  config: "a scalar string instead of a map"
)");

    YAML::Node defaults = YAML::Load(R"(
panel:
  config:
    nested: "value"
)");

    Structure structure("test_struct");
    structure.populate_widget_defaults(defaults);

    EXPECT_THROW(
        {
            try
            {
                structure.parse_file(configuration_file);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("Structural conflict between widget `defaults`"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Object References Resolved Numerically
 */
TEST_F(StructureTest, ObjectReferencesResolvedNumerically)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: root
  child:
    object: child_widget
child_widget:
  type: leaf
  name: "leaf node"
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);
    std::string const structure_json = structure.build(register_dependency_callback, true);

    auto const parsed = nlohmann::json::parse(structure_json);
    ASSERT_EQ(parsed["widgets"].size(), 2u);

    // Map keys in std::map are sorted alphabetically: "child_widget" (idx 0), "main" (idx 1)
    EXPECT_EQ(parsed["main"].get<int>(), 1);

    auto const &child_w = parsed["widgets"][0];
    auto const &main_w = parsed["widgets"][1];

    EXPECT_EQ(main_w[1]["child"]["object"].get<int>(), 0);
    EXPECT_EQ(child_w[1]["name"].get<std::string>(), "leaf node");
}

/**
 * @brief Test Object Reference Enforced Type Success
 */
TEST_F(StructureTest, ObjectReferenceEnforcedTypeSuccess)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: container
  child:
    object: !leaf_box my_leaf
my_leaf:
  type: leaf_box
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);
    EXPECT_NO_THROW(structure.build(register_dependency_callback));
}

/**
 * @brief Test Object Reference Enforced Type Mismatch Throws
 */
TEST_F(StructureTest, ObjectReferenceEnforcedTypeMismatchThrows)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: container
  child:
    object: !expected_type my_leaf
my_leaf:
  type: actual_type
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);

    EXPECT_THROW(
        {
            try
            {
                structure.build(register_dependency_callback);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("does not match the enforced type"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Object Reference Unknown Widget Throws
 */
TEST_F(StructureTest, ObjectReferenceUnknownWidgetThrows)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: container
  child:
    object: non_existent_widget
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);

    EXPECT_THROW(
        {
            try
            {
                structure.build(register_dependency_callback);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("does not relate to any known widgets"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Object Reference Null Throws
 */
TEST_F(StructureTest, ObjectReferenceNullThrows)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: container
  child:
    object: null
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);

    EXPECT_THROW(
        {
            try
            {
                structure.build(register_dependency_callback);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("No child object reference has been defined"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Object Reference Non Scalar Throws
 */
TEST_F(StructureTest, ObjectReferenceNonScalarThrows)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: container
  child:
    object:
      - item1
      - item2
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);

    EXPECT_THROW(
        {
            try
            {
                structure.build(register_dependency_callback);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("refers to a non-descriptive `object`"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Direct Self Recursion Detected
 */
TEST_F(StructureTest, DirectSelfRecursionDetected)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: recursive_node
  loop:
    object: main
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);

    EXPECT_THROW(
        {
            try
            {
                structure.build(register_dependency_callback);
            }
            catch (std::runtime_error const &error)
            {
                std::string const msg = error.what();
                EXPECT_NE(msg.find("Recursive object reference detected for `main`"), std::string::npos);
                EXPECT_NE(msg.find("(recursive reference)"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Indirect Recursion Cycle Detected
 */
TEST_F(StructureTest, IndirectRecursionCycleDetected)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: root
  child:
    object: node_a
node_a:
  type: intermediate
  child:
    object: node_b
node_b:
  type: intermediate
  child:
    object: node_a
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);

    EXPECT_THROW(
        {
            try
            {
                structure.build(register_dependency_callback);
            }
            catch (std::runtime_error const &error)
            {
                std::string const msg = error.what();
                EXPECT_NE(msg.find("Recursive object reference detected for `node_a`"), std::string::npos);
                EXPECT_NE(msg.find("(recursive reference)"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Orphaned Widgets And Types Pruned
 */
TEST_F(StructureTest, OrphanedWidgetsAndTypesPruned)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: used_root
  child:
    object: used_child
used_child:
  type: used_leaf
orphan_widget_1:
  type: orphan_type
orphan_widget_2:
  type: used_leaf
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);
    std::string const structure_json = structure.build(register_dependency_callback, false);

    auto const parsed = nlohmann::json::parse(structure_json);
    EXPECT_TRUE(parsed["widgets"].contains("main"));
    EXPECT_TRUE(parsed["widgets"].contains("used_child"));
    EXPECT_FALSE(parsed["widgets"].contains("orphan_widget_1"));
    EXPECT_FALSE(parsed["widgets"].contains("orphan_widget_2"));

    // Check that orphan_type was pruned from types
    auto const &types = parsed["types"];
    for (auto const &t : types)
    {
        EXPECT_NE(t.get<std::string>(), "orphan_type");
    }
}

/**
 * @brief Test File Tag Dependency Registration
 */
TEST_F(StructureTest, FileTagDependencyRegistration)
{
    temporary_directory.create_file("assets/image.png", "img_data");
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: image_viewer
  source: !file assets/image.png
)");

    std::string registered_path;
    PathRegisterDependency record_callback = [&](std::filesystem::path const &p) -> std::string
    {
        registered_path = p.lexically_normal().string();
        return "unique_hash.png";
    };

    Structure structure("test_struct");
    structure.parse_file(configuration_file);
    std::string const structure_json = structure.build(record_callback, false);

    auto const parsed = nlohmann::json::parse(structure_json);
    EXPECT_EQ(parsed["widgets"]["main"][1]["source"].get<std::string>(), "unique_hash.png");
    EXPECT_EQ(registered_path, (temporary_directory.path() / "assets/image.png").lexically_normal().string());
}

/**
 * @brief Test File Tag Empty Throws
 */
TEST_F(StructureTest, FileTagEmptyThrows)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: image_viewer
  source: !file ""
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);

    EXPECT_THROW(
        {
            try
            {
                structure.build(register_dependency_callback);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("Unable to add an (non-string) referenced dependency"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Multi File Dependency Loading
 */
TEST_F(StructureTest, MultiFileDependencyLoading)
{
    temporary_directory.create_file("widgets/buttons.yaml", R"(
my_button:
  type: btn
  label: "Click Me"
)");

    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
dependencies:
  - widgets/buttons.yaml
main:
  type: container
  btn:
    object: my_button
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);
    std::string const structure_json = structure.build(register_dependency_callback, false);

    auto const parsed = nlohmann::json::parse(structure_json);
    EXPECT_TRUE(parsed["widgets"].contains("main"));
    EXPECT_TRUE(parsed["widgets"].contains("my_button"));
    EXPECT_EQ(parsed["widgets"]["my_button"][1]["label"].get<std::string>(), "Click Me");
}

/**
 * @brief Test Circular File Dependencies Handled Gracefully
 */
TEST_F(StructureTest, CircularFileDependenciesHandledGracefully)
{
    temporary_directory.create_file("file_a.yaml", R"(
dependencies:
  - file_b.yaml
main:
  type: root
  child:
    object: widget_b
)");

    temporary_directory.create_file("file_b.yaml", R"(
dependencies:
  - file_a.yaml
widget_b:
  type: child
)");

    Structure structure("test_struct");
    // Should terminate and not stack overflow or infinite loop
    EXPECT_NO_THROW(structure.parse_file(temporary_directory.path() / "file_a.yaml"));
    EXPECT_NO_THROW(structure.build(register_dependency_callback));
}

/**
 * @brief Test Missing Dependency File Throws
 */
TEST_F(StructureTest, MissingDependencyFileThrows)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
dependencies:
  - missing_file.yaml
main:
  type: root
)");

    Structure structure("test_struct");
    EXPECT_THROW(
        {
            try
            {
                structure.parse_file(configuration_file);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("Unable to find dependency file of"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Non List Dependencies Throws
 */
TEST_F(StructureTest, NonListDependenciesThrows)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
dependencies: "should_be_a_list_not_a_string.yaml"
main:
  type: root
)");

    Structure structure("test_struct");
    EXPECT_THROW(
        {
            try
            {
                structure.parse_file(configuration_file);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("Unable to parse `dependencies` since a list is expected"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Multi Document Yaml Support
 */
TEST_F(StructureTest, MultiDocumentYamlSupport)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
---
main:
  type: root
  child:
    object: secondary_widget
---
secondary_widget:
  type: secondary
  val: 100
)");

    Structure structure("test_struct");
    structure.parse_file(configuration_file);
    std::string const structure_json = structure.build(register_dependency_callback, false);

    auto const parsed = nlohmann::json::parse(structure_json);
    EXPECT_TRUE(parsed["widgets"].contains("main"));
    EXPECT_TRUE(parsed["widgets"].contains("secondary_widget"));
    EXPECT_EQ(parsed["widgets"]["secondary_widget"][1]["val"].get<int>(), 100);
}

/**
 * @brief Test Non Mappable Document Throws
 */
TEST_F(StructureTest, NonMappableDocumentThrows)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
- item1
- item2
)");

    Structure structure("test_struct");
    EXPECT_THROW(
        {
            try
            {
                structure.parse_file(configuration_file);
            }
            catch (std::runtime_error const &error)
            {
                EXPECT_NE(std::string(error.what()).find("Unable to parse non-mappable structure"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

/**
 * @brief Test Debug Stream Output
 */
TEST_F(StructureTest, DebugStreamOutput)
{
    auto const configuration_file = temporary_directory.create_file("gui.yaml", R"(
main:
  type: root
)");

    std::stringstream debug_ss;
    Structure structure("CustomDebugName", &debug_ss);
    structure.parse_file(configuration_file);
    structure.build(register_dependency_callback);

    std::string output = debug_ss.str();
    EXPECT_NE(output.find("CustomDebugName: Parsing configuration dependency"), std::string::npos);
}
