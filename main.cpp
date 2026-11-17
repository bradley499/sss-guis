#include "guis.hpp"
#include <iostream>
#include <filesystem>
#include <vector>

using namespace sss::guis;

void print_help()
{
    std::cout << "Usage: sss-guis [OPTIONS] <CONFIGURATION_FILE> <OUTPUT_DIRECTORY>\n";
    std::cout << "Generate SSS powered GUIs from YAML files.\n";
    std::cout << "\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help                  Display this help message and exit\n";
    std::cout << "  -c, --allow-conflicts       Allow dependencies to conflict with generated outputs\n";
    std::cout << "  -f, --flatten-dependencies  Dependencies to not keep parent directory structure\n";
    std::cout << "  -v, --verbose               Enable verbose structural output\n";
    std::cout << "      --version               Show the version of the application\n";
    std::cout << "\n";
    std::cout << "Arguments:\n";
    std::cout << "  <CONFIGURATION_FILE>        A configuration file listing GUIs\n";
    std::cout << "  <OUTPUT_DIRECTORY>          The directory to output generated GUIs to\n";
}

bool show_help = false;
bool allow_conflicts = false;
bool flatten_dependencies = false;
std::ostream *verbose_stream = nullptr;
std::filesystem::path configuration_file;
std::filesystem::path output_directory;

void handle_arguments(int argc, char const *argv[])
{
    std::vector<std::filesystem::path> paths = {};
    for (int i = 1; i < argc; i++)
    {
        std::string argument = argv[i];
        if (argument.length() >= 2 && argument[0] == '-')
        {
            if (argument[1] == '-')
            {
                if (argument == "--help")
                    show_help = true;
                else if (argument == "--allow-conflicts")
                    allow_conflicts = true;
                else if (argument == "--flatten-dependencies")
                    flatten_dependencies = true;
                else if (argument == "--verbose")
                    verbose_stream = &(std::cout);
                else if (argument == "--version")
                {
                    std::cout << "sss-guis: v" << SSS_GUIS_VERSION_MAJOR << "." << SSS_GUIS_VERSION_MINOR << "." << SSS_GUIS_VERSION_PATCH << "\n";
                    exit(EXIT_SUCCESS);
                }
                else
                {
                    std::cerr << "sss-guis: Unrecognized option -- '" << argument << "'\n";
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                for (size_t ii = 1; ii < argument.length(); ii++)
                {
                    char character = argument[ii];
                    switch (character)
                    {
                    case 'h':
                        show_help = true;
                        break;
                    case 'c':
                        allow_conflicts = true;
                        break;
                    case 'f':
                        flatten_dependencies = true;
                        break;
                    case 'v':
                        verbose_stream = &(std::cout);
                        break;
                    default:
                        std::cerr << "sss-guis: Invalid option -- '" << character << "'\n";
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        else
            paths.push_back(std::filesystem::path(argument));
    }
    if (show_help)
    {
        print_help();
        exit(EXIT_FAILURE);
    }
    if (paths.size() < 2)
    {
        std::cerr << "sss-guis: Too few arguments\n";
        exit(EXIT_FAILURE);
    }
    else if (paths.size() > 2)
    {
        std::cerr << "sss-guis: Too many arguments\n";
        exit(EXIT_FAILURE);
    }
    configuration_file = paths[0];
    output_directory = paths[1];
}

int main(int argc, char const *argv[])
{
    handle_arguments(argc, argv);
    try
    {
        Guis(configuration_file, output_directory).generate(!allow_conflicts, flatten_dependencies, verbose_stream);
        return EXIT_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cerr << "sss-guis: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
