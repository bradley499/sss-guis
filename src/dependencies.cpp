#include "dependencies.hpp"
#include <regex>
#include <string>

using namespace sss::guis;

namespace
{
    /**
     * @brief Converts a glob pattern to a regular expression
     * @param pattern Glob pattern
     * @returns Regular expression
     */
    inline std::regex const glob_to_regex(std::string const &pattern)
    {
        std::string regex_pattern;
        for (char character : pattern)
        {
            if (character == '*')
                regex_pattern += ".*";
            else if (character == '?')
                regex_pattern += ".";
            else if (character == '.')
                regex_pattern += "\\.";
            else if (character == '\\')
                regex_pattern += "\\\\";
            else if (character == '/')
                regex_pattern += "\\/";
            else
                regex_pattern += character;
        }
        return std::regex(regex_pattern);
    }

    /**
     * @brief Get the directory path prior to wildcard
     * @param wildcard_path Path to validate against
     * @returns Directory path prior to wildcard
     */
    inline std::filesystem::path const get_starting_directory_from_wildcard_path(std::filesystem::path const &wildcard_path)
    {
        std::filesystem::path current_directory;
        for (auto const &component : wildcard_path)
        {
            if (component.string().find_first_of("*?") != std::string::npos)
                break; // Found a wildcard component, the current_directory is the starting directory
            current_directory /= component;
        }
        return current_directory;
    }
}

dependencies_t::dependencies_t(std::filesystem::path const &path)
    : m_path(path)
{
}

dependencies_t::~dependencies_t()
{
}

std::vector<std::filesystem::path> dependencies_t::paths()
{
    if (std::filesystem::exists(m_path))
        return {m_path};
    std::vector<std::filesystem::path> existing_paths = {};

    // Convert the glob pattern to a regex pattern
    std::regex const path_regex = glob_to_regex(m_path);
    std::filesystem::path const start_directory = get_starting_directory_from_wildcard_path(m_path);
    try
    {
        for (auto const &entry : std::filesystem::recursive_directory_iterator(start_directory))
        {
            std::filesystem::path relative_entry_path = std::filesystem::relative(entry.path(), start_directory);
            if (std::regex_match((start_directory / relative_entry_path).string(), path_regex) && std::filesystem::is_regular_file(entry.path()))
                    existing_paths.push_back(entry.path());
        }
    }
    catch (std::filesystem::filesystem_error const &e)
    {
        throw std::runtime_error("Failed to load dependency path");
    }
    return existing_paths;
}
