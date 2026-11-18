#pragma once

#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <map>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace sss::guis
{
    /**
     * @brief Callback function to register a path as a dependency
     */
    using PathRegisterDependency = std::function<std::string(std::filesystem::path const &)>;

    class Structure
    {
    private:
        /**
         * @brief The name of a widget
         */
        using WidgetName = std::string;
        /**
         * @brief Numeric reference for a type
         */
        using WidgetTypeIdentifier = int;
        /**
         * @brief The type of a widget
         */
        using WidgetType = std::string;
        /**
         * @brief The contents of a widget
         */
        using WidgetContents = YAML::Node;
        /**
         * @brief The reference to a file that declares a widget
         */
        using WidgetFileReference = int;
        /**
         * @brief Widget names and type identifiers
         */
        std::map<WidgetName, WidgetTypeIdentifier> m_widgets;
        /**
         * @brief Collection of widget type
         */
        std::vector<WidgetType> m_widget_types;
        /**
         * @brief Collection of widget names and their contents
         */
        std::map<WidgetName, WidgetContents> m_widget_contents;
        /**
         * @brief Collection of widget types and their default contents
         */
        std::map<WidgetType, WidgetContents> m_widget_contents_defaults;
        /**
         * @brief Collection of widget names and their declaration file
         */
        std::map<WidgetName, WidgetFileReference> m_widget_files;
        /**
         * @brief Collection of already parsed files
         */
        std::vector<std::filesystem::path> m_parsed_files;
        /**
         * @brief The name of this structure (only used for debug output)
         */
        std::string const m_name;
        /**
         * @brief Output stream for debug messages
         */
        std::ostream *m_debug_stream;
        /**
         * @brief Add a widget reference
         * @param name The name of the widget
         * @param type The type of the widget
         * @param contents The remaining contents of the widget
         * @param file_reference The reference to the file of the widget's definition
         */
        void add_widget(WidgetName const &name, WidgetType const &type, YAML::Node const &widget_base_node, WidgetContents const &contents, WidgetFileReference const file_reference);
        /**
         * @brief Convert object references to numerical references
         */
        void number_references();
        /**
         * @brief Remove all unreferenced objects and validate object hierarchical structure and dependencies
         * @param path_register_dependency_callback Callback function to register a path as a dependency
         */
        void validate_and_prune_hierarchy(PathRegisterDependency const &path_register_dependency_callback);

    public:
        /**
         * @brief Construct a structure parser object that parsers YAML files into a JSON object
         * @param name The name of the structure (only used for debug output)
         * @param debug_stream A `std::ofstream` to write debug outputs to
         */
        Structure(std::string const &name, std::ostream const *debug_stream = nullptr);
        /**
         * @brief Deconstructor
         */
        ~Structure();
        /**
         * @brief Populate widgets with default properties
         * @param widget_defaults A collection of default values for widgets of defined types
         */
        void populate_widget_defaults(WidgetContents const &widget_defaults);
        /**
         * @brief Parse a YAML file, handling dependencies and widgets
         * @param configuration_file The source configuration file to start structuring from
         */
        void parse_file(std::filesystem::path const &configuration_file);
        /**
         * @brief Build JSON output
         * @param path_register_dependency_callback Callback function to register a path as a dependency
         * @param numeric_references Whether to convert object references to a numeric value
         */
        std::string build(PathRegisterDependency const &path_register_dependency_callback, bool const numeric_references = true);
    };
}
