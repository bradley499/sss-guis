#pragma once

#include <filesystem>

namespace sss::guis
{
    class Guis
    {
    private:
        /**
         * @brief The output directory for all generated file
         */
        std::filesystem::path const m_configuration_directory;
        /**
         * @brief The source configuration file to find structures in
         */
        std::filesystem::path const m_output_directory;

    public:
        /**
         * @brief Construct a GUI generator
         * @param configuration_file The source configuration file to find structures in
         * @param output_directory The output directory for all generated file
         */
        Guis(std::filesystem::path const &configuration_file, std::filesystem::path const &output_directory);
        /**
         * @brief Deconstructor
         */
        ~Guis();
        /**
         * @brief Generate GUIs
         * @param disallow_conflicts Do not allow dependencies or generated file to have conflicting output file name
         * @param flatten_dependency_references Whether to flatten dependency output files to just their filename (no directory hierarchy)
         * @param debug_stream A `std::ofstream` to write debug outputs to
         */
        void generate(bool const disallow_conflicts = true, bool const flatten_dependency_references = false, std::ostream const *debug_stream = nullptr);
    };
    extern Guis guis;
}
