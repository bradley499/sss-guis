#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include "structure.hpp"

namespace sss::guis
{
    class generation_t
    {
    private:
        /**
         * @brief The structure of a GUI (prior to processing)
         */
        struct gui_t
        {
            /**
             * @brief The name of the GUI
             */
            std::string name;
            /**
             * @brief Whether the GUI should be generated with debug mode
             */
            bool debug;
            /**
             * @brief The initial configuration file for the GUI
             */
            std::string source_configuration_file;
            /**
             * @brief Path of stylesheet
             */
            std::string stylesheet_file;
            /**
             * @brief Output file path of HTML file
             */
            std::string html_file;
            /**
             * @brief Output file path of generated structure
             */
            std::string structure_file;
            /**
             * @brief Paths of modules
             */
            std::vector<std::string> module_files;
            /**
             * @brief A map of YAML nodes consisting of the default values for defined widget type
             */
            std::optional<YAML::Node> widget_defaults;
        };
        /**
         * @brief Collection of all GUI's data
         */
        std::vector<gui_t> m_guis;
        /**
         * @brief Collection of all dependencies
         */
        std::map<std::filesystem::path, std::filesystem::path> m_dependencies;
        /**
         * @brief The output directory for all generated file
         */
        std::filesystem::path const m_configuration_directory;
        /**
         * @brief The path to the configuration file
         */
        std::filesystem::path const m_configuration_file;
        /**
         * @brief The source configuration file to find structures in
         */
        std::filesystem::path const m_output_directory;
        /**
         * @brief Generates a GUI
         * @param gui The GUI to generate
         * @param guis_js_path The path to the main JavaScript file
         * @param register_dependency_callback Callback function to register a path as a dependency
         * @param debug_stream A `std::ofstream` to write debug outputs to
         */
        void generate(gui_t const &gui, std::string const &guis_js_path, path_register_dependency_t const &register_dependency_callback, std::ostream const *debug_stream = nullptr);
        /**
         * @brief Generate a unique filename against the dependencies with a specified extension
         * @param extension The extension to use for the unique file (expected to begin with `.`)
         * @return Unique file (with extension)
         */
        std::filesystem::path unique_filename(std::string const &extension);

    public:
        /**
         * @brief Construct a GUI generator
         * @param configuration_file The source configuration file to find structures in
         * @param output_directory The output directory for all generated file
         */
        generation_t(std::filesystem::path const &configuration_file, std::filesystem::path const &output_directory);
        /**
         * @brief Deconstructor
         */
        ~generation_t();
        /**
         * @brief Build all GUIs and manage dependencies
         * @param disallow_conflicts Do not allow dependencies or generated file to have conflicting output file name
         * @param flatten_dependency_references Whether to flatten dependency output files to just their filename (no directory hierarchy)
         * @param debug_stream A `std::ofstream` to write debug outputs to
         */
        void build_all(bool const disallow_conflicts = true, bool const flatten_dependency_references = false, std::ostream const *debug_stream = nullptr);
    };
}
