#pragma once

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace sss::guis
{
    /**
     * @brief RAII managed temporary directory for testing
     */
    class TemporaryDirectory
    {
    private:
        /**
         * @brief Canonical path to the temporary directory
         */
        std::filesystem::path m_path;

    public:
        /**
         * @brief Create a unique temporary directory
         * @param prefix Prefix for the directory name
         */
        TemporaryDirectory(std::string const &prefix = "sss_guis_test")
        {
            auto const temporary_base = std::filesystem::temp_directory_path();
            auto const now = std::chrono::steady_clock::now().time_since_epoch().count();
            static std::mt19937_64 random_number_generator(static_cast<uint64_t>(now));
            std::uniform_int_distribution<uint64_t> distribution;

            for (int attempt = 0; attempt < 50; ++attempt)
            {
                auto const candidate = temporary_base / (prefix + "_" + std::to_string(now) + "_" + std::to_string(distribution(random_number_generator)));
                if (!std::filesystem::exists(candidate))
                {
                    if (std::filesystem::create_directories(candidate))
                    {
                        m_path = std::filesystem::canonical(candidate);
                        return;
                    }
                }
            }
            throw std::runtime_error("Failed to create temporary directory for test");
        }

        /**
         * @brief Remove the temporary directory and all contents
         */
        ~TemporaryDirectory()
        {
            std::error_code error_code;
            if (!m_path.empty() && std::filesystem::exists(m_path, error_code))
                std::filesystem::remove_all(m_path, error_code);
        }

        TemporaryDirectory(TemporaryDirectory const &) = delete;
        TemporaryDirectory &operator=(TemporaryDirectory const &) = delete;

        TemporaryDirectory(TemporaryDirectory &&other) noexcept
            : m_path(std::move(other.m_path))
        {
            other.m_path.clear();
        }

        TemporaryDirectory &operator=(TemporaryDirectory &&other) noexcept
        {
            if (this != &other)
            {
                std::error_code error_code;
                if (!m_path.empty() && std::filesystem::exists(m_path, error_code))
                    std::filesystem::remove_all(m_path, error_code);
                m_path = std::move(other.m_path);
                other.m_path.clear();
            }
            return *this;
        }

        /**
         * @brief Get the temporary directory path
         * @returns Temporary directory path
         */
        std::filesystem::path const &path() const
        {
            return m_path;
        }

        /**
         * @brief Create a file with contents in the temporary directory
         * @param relative Relative path of file
         * @param content Contents to write to file
         * @returns Full path of created file
         */
        std::filesystem::path create_file(std::filesystem::path const &relative, std::string const &content = "") const
        {
            auto const full_path = m_path / relative;
            std::filesystem::create_directories(full_path.parent_path());
            std::ofstream output_file_stream(full_path, std::ios::binary);
            if (!output_file_stream.is_open())
                throw std::runtime_error("Failed to create file: " + full_path.string());
            output_file_stream << content;
            return full_path;
        }

        /**
         * @brief Create a directory in the temporary directory
         * @param relative Relative path of directory
         * @returns Full path of created directory
         */
        std::filesystem::path create_directory(std::filesystem::path const &relative) const
        {
            auto const full_path = m_path / relative;
            std::filesystem::create_directories(full_path);
            return full_path;
        }

        /**
         * @brief Read a file from the temporary directory
         * @param relative Relative path of file
         * @returns File contents as string
         */
        std::string read_file(std::filesystem::path const &relative) const
        {
            auto const full_path = m_path / relative;
            std::ifstream input_file_stream(full_path, std::ios::binary);
            if (!input_file_stream.is_open())
                throw std::runtime_error("Failed to open file for reading: " + full_path.string());
            std::stringstream buffer;
            buffer << input_file_stream.rdbuf();
            return buffer.str();
        }

        /**
         * @brief Check if a relative path exists in the temporary directory
         * @param relative Relative path
         * @returns Whether path exists
         */
        bool exists(std::filesystem::path const &relative) const
        {
            return std::filesystem::exists(m_path / relative);
        }
    };

    /**
     * @brief Extract embedded GUI metadata JSON from generated HTML
     * @param html_content The HTML content
     * @returns Parsed JSON metadata
     */
    inline nlohmann::json extract_gui_json_from_html(std::string const &html_content)
    {
        std::regex const gui_regex(R"(const gui=(\{.*?\});)");
        std::smatch match;
        if (std::regex_search(html_content, match, gui_regex))
            return nlohmann::json::parse(match[1].str());
        throw std::runtime_error("Could not find embedded 'const gui=...' JSON in HTML");
    }

    /**
     * @brief Read and parse structure JSON referenced by generated HTML
     * @param output_directory The output directory of the GUI
     * @param html_content The HTML content containing structure reference
     * @returns Parsed structure JSON
     */
    inline nlohmann::json read_structure_json_from_html(std::filesystem::path const &output_directory, std::string const &html_content)
    {
        auto const gui_metadata = extract_gui_json_from_html(html_content);
        std::string const structure_relative_path = gui_metadata["structure"].get<std::string>();
        std::filesystem::path const structure_path = (output_directory / structure_relative_path).lexically_normal();
        if (!std::filesystem::exists(structure_path))
            throw std::runtime_error("Structure file does not exist at: " + structure_path.string());
        std::ifstream input_file_stream(structure_path);
        return nlohmann::json::parse(input_file_stream);
    }
}
