#pragma once

#include <filesystem>
#include <vector>

namespace sss::guis
{
    class Dependencies
    {
    private:
        /**
         * @brief The path to evaluate against
         */
        std::filesystem::path const m_path;

    public:
        /**
         * @brief Construct a dependency tree
         * @param path The path to evaluate (support * and ? wildcards)
         */
        Dependencies(std::filesystem::path const &path);
        /**
         * @brief Deconstructor
         */
        ~Dependencies();
        /**
         * @brief Show all evaluated paths
         * @returns A vector of all matching (existing) paths
         */
        std::vector<std::filesystem::path> paths();
    };
}
