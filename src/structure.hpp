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
    using path_register_dependency_t = std::function<std::string(std::filesystem::path const &)>;

    class structure_t
    {
    private:
        /**
         * @brief The name of a widget
         */
        using widget_name_t = std::string;
        /**
         * @brief Numeric reference for a type
         */
        using widget_type_identifier_t = int;
        /**
         * @brief The type of a widget
         */
        using widget_type_t = std::string;
        /**
         * @brief The contents of a widget
         */
        using widget_contents_t = YAML::Node;
        /**
         * @brief The reference to a file that declares a widget
         */
        using widget_file_reference_t = int;
        /**
         * @brief Widget names and type identifiers
         */
        std::map<widget_name_t, widget_type_identifier_t> m_widgets;
        /**
         * @brief Collection of widget type
         */
        std::vector<widget_type_t> m_widget_types;
        /**
         * @brief Collection of widget names and their contents
         */
        std::map<widget_name_t, YAML::Node> m_widget_contents;
        /**
         * @brief Collection of widget names and their declaration file
         */
        std::map<widget_name_t, widget_file_reference_t> m_widget_files;
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
         * @brief Parse a YAML file, handling dependencies and widgets
         * @param file The YAML file to parse
         */
        void parse_file(std::filesystem::path const &file);
        /**
         * @brief Add a widget reference
         * @param name The name of the widget
         * @param type The type of the widget
         * @param contents The remaining contents of the widget
         */
        void add_widget(widget_name_t const &name, widget_type_t const &type, widget_contents_t const &contents, widget_file_reference_t const file_reference);
        /**
         * @brief Convert object references to numerical references
         */
        void number_references();
        /**
         * @brief Remove all unreferenced objects and validate object hierarchical structure and dependencies
         * @param path_register_dependency_callback Callback function to register a path as a dependency
         */
        void validate_and_prune_hierarchy(path_register_dependency_t const &path_register_dependency_callback);

    public:
        /**
         * @brief Construct a structure parser object that parsers YAML files into a JSON object
         * @param configuration_file The source configuration file to start structuring from
         * @param name The name of the structure (only used for debug output)
         * @param debug_stream A `std::ofstream` to write debug outputs to
         */
        structure_t(std::string const &configuration_file, std::string const &name, std::ostream const *debug_stream = nullptr);
        /**
         * @brief Deconstructor
         */
        ~structure_t();
        /**
         * @brief Build JSON output
         * @param path_register_dependency_callback Callback function to register a path as a dependency
         * @param numeric_references Whether to convert object references to a numeric value
         */
        std::string build(path_register_dependency_t const &path_register_dependency_callback, bool const numeric_references = true);
    };
}
