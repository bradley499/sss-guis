#include "dependencies.hpp"
#include "generation.hpp"
#include "guis.js.hpp" // Generated file

#include <algorithm>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <streambuf>
#include <thread>
#include <yaml-cpp/yaml.h>

using namespace sss::guis;

namespace
{
    /**
     * @brief The default project name
     */
    std::string const project_default = "SSS";

    /**
     * @brief The default project delimiter
     */
    std::string const project_delimiter_default = " | ";

    /**
     * @brief Check if a string is empty
     * @param string The string to check
     * @returns Whether the string is empty
     */
    inline bool is_empty(std::string const &string)
    {
        return std::all_of(string.begin(), string.end(), [](unsigned char character)
                           { return std::isspace(character); });
    }

    /**
     * @brief Checks if a filepath is a descendant of another filepath
     * @param parent Filepath
     * @param child Filepath
     * @returns If the child path is a descendant of the parent
     */
    inline bool is_descendant(std::filesystem::path const &parent, std::filesystem::path const &child)
    {
        auto const relative_path = child.lexically_normal().lexically_relative(parent.lexically_normal());
        return (!relative_path.empty() && relative_path.native()[0] != '.' && relative_path != ".");
    }

    /**
     * @brief Removes any filepaths that are descendants of any filepaths in a vector
     * @param paths A vector of filepaths
     * @returns A vector of filepaths with descendant filepaths removed
     */
    inline std::vector<std::filesystem::path> const remove_descendant_paths(std::vector<std::filesystem::path> &paths)
    {
        // Sort the paths to ensure parent paths come before their descendants
        std::sort(paths.begin(), paths.end());

        std::vector<std::filesystem::path> unique_paths = {};
        if (paths.empty())
            return unique_paths;

        unique_paths.push_back(paths[0]); // Add the first path as it's the most "parental" initially

        for (auto const &path : paths)
        {
            if (std::find(unique_paths.begin(), unique_paths.end(), path) != unique_paths.end())
                continue;
            // Check if the current path is a descendant of the last unique path found
            if (!is_descendant(unique_paths.back(), path))
                unique_paths.push_back(path); // Keep if not a descendant
        }
        return unique_paths;
    }

    /**
     * @brief Convert a relative filepath to an absolute filepath
     * @param target_path The filepath to make absolute against `base_path`
     * @param base_path The base filepath to perform evaluation against
     * @return Absolute filepath
     */
    std::filesystem::path convert_relative_path_to_absolute(std::filesystem::path const &target_path, std::filesystem::path const &base_path)
    {
        std::filesystem::path base_directory = std::filesystem::absolute(base_path).parent_path();
        std::filesystem::path absolute_target_path;
        if (target_path.is_absolute())
            absolute_target_path = target_path;
        else
            absolute_target_path = base_directory / target_path;
        return std::filesystem::absolute(base_directory / std::filesystem::relative(absolute_target_path, base_directory)).lexically_normal();
    }

    /**
     * @brief Get a filepath relative to another
     * @param target_path The filepath to make absolute against `base_path`
     * @param base_path The base filepath to perform evaluation against
     * @return Relative filepath
     */
    std::filesystem::path convert_absolute_path_to_relative(std::filesystem::path const &target_path, std::filesystem::path const &base_path)
    {
        std::filesystem::path filepath;
        for (auto const &directory : std::filesystem::relative(std::filesystem::absolute(target_path).lexically_normal(), base_path))
        {
            if (directory == ".." || directory == ".")
                continue;
            filepath /= directory; // Join remaining parts
        }
        return filepath;
    }

    /**
     * @brief Calculates the amount of directories that a name would span into
     * @param name The name to calculate against
     * @return Directory depth count
     */
    inline int name_as_path_depth(std::string const &name)
    {
        std::filesystem::path name_path(name);
        int count = (std::distance(name_path.begin(), name_path.end()) - 1);
        if (name_path.has_root_directory())
            count--;
        if (name_path.has_root_name() && !name_path.has_root_directory())
            count--;
        if (count < 0)
            count = 0;
        return count;
    }

    /**
     * @brief Sanitize name for use in filesystem
     * @param name The name to sanitize
     * @return Sanitized name
     */
    std::string sanitize_name(std::string const &name)
    {
        std::filesystem::path original(name);
        std::filesystem::path result;

        for (auto const &part : original)
        {
            if (part == std::filesystem::path("/") || part == ".." || part == ".")
                continue;
            result /= part;
        }

        return result.string();
    }

    /**
     * @brief Get the line number of a YAML Mark object
     * @param mark The YAML Mark object to get the line number from
     * @returns The line number of the YAML Mark object
     */
    inline int yaml_mark_line_number(YAML::Mark const &mark)
    {
        return (mark.line + 1);
    }

    /**
     * @brief Get the line number of a YAML Node object
     * @param node The YAML Node object to get the line number from
     * @return The line number of the YAML Node object
     */
    inline int yaml_node_line_number(YAML::Node const &node)
    {
        return yaml_mark_line_number(node.Mark());
    }
}

generation_t::generation_t(std::filesystem::path const &configuration_file, std::filesystem::path const &output_directory)
    : m_guis({}),
      m_dependencies({}),
      m_configuration_directory(std::filesystem::absolute(configuration_file.lexically_normal()).parent_path()),
      m_configuration_file(configuration_file),
      m_output_directory(std::filesystem::absolute(output_directory.lexically_normal()))
{
    std::vector<std::tuple<YAML::Node, YAML::Node, YAML::Node>> gui_nodes = {};
    try
    {
        for (auto const &document : YAML::LoadAllFromFile(configuration_file.string()))
        {
            if (!document.IsDefined())
                continue;
            YAML::Node const project_name = document["project"];
            YAML::Node const project_name_delimiter = document["project_delimiter"];
            YAML::Node const guis = document["guis"];
            if (!guis.IsDefined() || !guis.IsSequence())
                continue;
            for (auto &&gui : guis)
                gui_nodes.emplace_back(gui, project_name, project_name_delimiter);
        }
        if (gui_nodes.empty())
            throw std::runtime_error("Expected list of `guis` in configuration");
    }
    catch (YAML::Exception const &e)
    {
        throw std::runtime_error("Failed to load descriptive YAML file \"" + configuration_file.string() + "\" due to an error on line " + std::to_string(yaml_mark_line_number(e.mark)) + ": " + e.msg);
    }
    for (auto const &[gui_node, project_name, project_delimiter] : gui_nodes)
    {
        if (!gui_node.IsMap())
            throw std::runtime_error("Expected `gui` to hold a defined configuration on line " + std::to_string(yaml_node_line_number(gui_node)) + " of \"" + configuration_file.string() + "\"");
        gui_t current_gui_data = {};

        // Check whether core configuration strings are present
        for (std::string const field : {"name", "config", "stylesheet"})
        {
            if (!gui_node[field].IsDefined())
                throw std::runtime_error("Required field of `" + field + "` is missing for a gui as defined on line " + std::to_string(yaml_node_line_number(gui_node[field])) + " of \"" + configuration_file.string() + "\"");
            if (gui_node[field].IsScalar())
            {
                try
                {
                    gui_node[field].as<std::string>();
                    continue;
                }
                catch (YAML::BadConversion const &e)
                {
                }
            }
            throw std::runtime_error("Unable to parse required field of `" + field + "` since a string is expected on line " + std::to_string(yaml_node_line_number(gui_node[field])) + " of \"" + configuration_file.string() + "\"");
        }

        // Store name of GUI
        auto const name = gui_node["name"];
        current_gui_data.name = name.as<std::string>();
        if (is_empty(current_gui_data.name))
            throw std::runtime_error("A name must not be empty on line " + std::to_string(yaml_node_line_number(name)) + " of \"" + configuration_file.string() + "\"");

        // Store name of project
        try
        {
            current_gui_data.project = project_name.as<std::string>(project_default);
        }
        catch (YAML::BadConversion const &e)
        {
            throw std::runtime_error("Invalid `project` name only a string is expected on line " + std::to_string(yaml_node_line_number(project_name)) + " of \"" + configuration_file.string() + "\"");
        }
        if (is_empty(current_gui_data.project))
            throw std::runtime_error("Invalid `project` name must not be empty on line " + std::to_string(yaml_node_line_number(project_name)) + " of \"" + configuration_file.string() + "\"");

        // Store project delimiter (this is allowed to be empty)
        try
        {
            current_gui_data.project_delimiter = project_delimiter.as<std::string>(project_delimiter_default);
        }
        catch (YAML::BadConversion const &e)
        {
            throw std::runtime_error("Invalid `project_delimiter` name only a string is expected on line " + std::to_string(yaml_node_line_number(project_delimiter)) + " of \"" + configuration_file.string() + "\"");
        }

        // Store configuration filepath of GUI
        auto const config = gui_node["config"];
        current_gui_data.source_configuration_file = config.as<std::string>();
        current_gui_data.source_configuration_file = convert_relative_path_to_absolute(current_gui_data.source_configuration_file, configuration_file).string();
        if (current_gui_data.source_configuration_file.empty() || !std::filesystem::exists(current_gui_data.source_configuration_file))
            throw std::runtime_error("Unable to find a source config file of \"" + current_gui_data.source_configuration_file + "\" as defined on line " + std::to_string(yaml_node_line_number(config)) + " of \"" + configuration_file.string() + "\"");

        // Store stylesheet filepath of GUI
        auto const stylesheet = gui_node["stylesheet"];
        current_gui_data.stylesheet_file = convert_relative_path_to_absolute(stylesheet.as<std::string>(), configuration_file);
        std::filesystem::path stylesheet_path = std::filesystem::absolute(current_gui_data.stylesheet_file).lexically_normal();
        if (!std::filesystem::exists(stylesheet_path.string()))
            throw std::runtime_error("Unable to find the stylesheet \"" + current_gui_data.stylesheet_file + "\" defined on line " + std::to_string(yaml_node_line_number(stylesheet)) + " of \"" + configuration_file.string() + "\"");
        current_gui_data.stylesheet_file = convert_absolute_path_to_relative(stylesheet_path, m_configuration_directory);
        m_dependencies[std::filesystem::weakly_canonical(stylesheet_path)] = current_gui_data.stylesheet_file;

        // Set html filepath of GUI
        current_gui_data.html_file = sanitize_name(current_gui_data.name) + ".html";
        if (std::filesystem::exists(m_output_directory / current_gui_data.html_file))
            throw std::runtime_error("Unable to generate source for \"" + current_gui_data.html_file + "\" as a file already exists with that name");

        // Store debug state of GUI
        current_gui_data.debug = false;
        YAML::Node const debug = gui_node["debug"];
        if (debug.IsDefined())
        {
            do
            {
                if (debug.IsScalar())
                {
                    try
                    {
                        current_gui_data.debug = debug.as<bool>(false);
                        continue;
                    }
                    catch (std::exception const &e)
                    {
                    }
                }
                throw std::runtime_error("Unable to parse `debug` since a boolean value is expected on line " + std::to_string(yaml_node_line_number(debug)) + " of \"" + configuration_file.string() + "\"");
            } while (false);
        }
        // Check whether modules are listed
        current_gui_data.module_files = {};
        YAML::Node const modules = gui_node["modules"];
        if (modules.IsDefined())
        {
            // Expect a list of modules
            if (modules.IsSequence())
            {
                // Get the directory of the file being processed
                for (YAML::Node const &module : modules)
                {
                    if (!module.IsScalar())
                        throw std::runtime_error("Expected a string path for a module on line " + std::to_string(yaml_node_line_number(module)) + " of \"" + configuration_file.string() + "\"");

                    // Convert the dependency string to a std::filesystem::path object
                    std::filesystem::path module_path;
                    try
                    {
                        module_path = convert_relative_path_to_absolute(module.as<std::string>(), configuration_file);
                    }
                    catch (YAML::BadConversion const &e)
                    {
                        throw std::runtime_error("Expected a string path for a module on line " + std::to_string(yaml_node_line_number(module)) + " of \"" + configuration_file.string() + "\"");
                    }
                    if (std::filesystem::exists(module_path) && std::filesystem::is_regular_file(module_path))
                    {
                        std::filesystem::path module_path_relative = convert_absolute_path_to_relative(module_path, m_configuration_directory);
                        m_dependencies[std::filesystem::weakly_canonical(module_path.lexically_relative(m_configuration_directory))] = module_path_relative;
                        current_gui_data.module_files.push_back(module_path_relative);
                    }
                    else
                        throw std::runtime_error("No module file exists at \"" + module_path.string() + "\" as defined on line " + std::to_string(yaml_node_line_number(module)) + " of \"" + configuration_file.string() + "\"");
                }
            }
        }

        // Check whether dependencies are listed
        YAML::Node const dependencies = gui_node["dependencies"];
        if (dependencies.IsDefined())
        {
            // Expect a list of dependencies
            if (dependencies.IsSequence())
            {
                // Get the directory of the file being processed
                for (YAML::Node const &dependency : dependencies)
                {
                    if (!dependency.IsScalar())
                        throw std::runtime_error("Expected a string path for a dependency on line " + std::to_string(yaml_node_line_number(dependency)) + " of \"" + configuration_file.string() + "\"");

                    // Convert the dependency string to a std::filesystem::path object
                    std::string dependency_path;
                    try
                    {
                        dependency_path = convert_relative_path_to_absolute(dependency.as<std::string>(), configuration_file);
                    }
                    catch (YAML::BadConversion const &e)
                    {
                        throw std::runtime_error("Expected a string path for a dependency on line " + std::to_string(yaml_node_line_number(dependency)) + " of \"" + configuration_file.string() + "\"");
                    }
                    try
                    {
                        for (auto &&dependency : dependencies_t(dependency_path).paths())
                            m_dependencies[std::filesystem::weakly_canonical(dependency.lexically_relative(m_configuration_directory))] = convert_absolute_path_to_relative(dependency, m_configuration_directory);
                    }
                    catch (std::exception const &e)
                    {
                        throw std::runtime_error("No file(s) exists for dependency \"" + dependency_path + "\" within the configuration directory as defined on line " + std::to_string(yaml_node_line_number(dependency)) + " of \"" + configuration_file.string() + "\"");
                    }
                }
            }
            else if (dependencies.Type() != YAML::NodeType::Null)
                throw std::runtime_error("Unable to parse `dependencies` since a list is expected on line " + std::to_string(yaml_node_line_number(dependencies)) + " of \"" + configuration_file.string() + "\"");
        }

        // Check whether widget defaults are listed
        YAML::Node const widget_defaults = gui_node["defaults"];
        if (widget_defaults.IsDefined())
        {
            // Expect a list of dependencies
            if (!widget_defaults.IsMap())
                throw std::runtime_error("Unable to parse `defaults` since a mappable structure is expected on line " + std::to_string(yaml_node_line_number(widget_defaults)) + " of \"" + configuration_file.string() + "\"");
            current_gui_data.widget_defaults = widget_defaults;
        }
        m_guis.push_back(current_gui_data); // Add to the collection
    }
    if (!std::filesystem::exists(m_output_directory))
    {
        if (!std::filesystem::create_directories(m_output_directory))
            throw std::runtime_error("Failed to create output directory");
    }
    else if (!std::filesystem::is_directory(m_output_directory))
        throw std::runtime_error("There is already a file located as the output directory location");
}

generation_t::~generation_t()
{
    m_guis.clear();
    m_dependencies.clear();
}

void generation_t::generate(generation_t::gui_t const &data, std::string const &guis_js_path, path_register_dependency_t const &register_dependency_callback, std::ostream const *debug_stream)
{
    std::string const relative_adjustment = [&]
    {
        std::string parent_path;
        for (auto i = 0; i < name_as_path_depth(sanitize_name(data.name)); ++i)
            parent_path += "../";
        return parent_path;
    }();

    path_register_dependency_t const register_dependency_callback_wrapper = [relative_adjustment, register_dependency_callback](std::filesystem::path const &file_path)
    {
        return relative_adjustment + register_dependency_callback(file_path);
    };

    std::string structure;
    try
    {
        // Generate structure
        structure_t structure_parser(data.name, debug_stream);
        if (data.widget_defaults.has_value())
        {
            try
            {
                structure_parser.populate_widget_defaults(data.widget_defaults.value());
            }
            catch (std::runtime_error const &e)
            {
                throw std::runtime_error(std::string(e.what()) + " of \"" + m_configuration_file.string() + "\"");
            }
        }
        structure_parser.parse_file(std::filesystem::absolute(data.source_configuration_file).lexically_normal());
        structure = structure_parser.build(register_dependency_callback_wrapper, !data.debug);
    }
    catch (std::exception const &e)
    {
        throw std::runtime_error(data.name + ": " + e.what());
    }

    // Structure output filepath
    std::string const structure_file = unique_filename(".json");

    std::vector<std::string> modules = {};
    std::transform(data.module_files.begin(), data.module_files.end(), std::back_inserter(modules),
                   [relative_adjustment](std::string const &module_file)
                   {
                       return relative_adjustment + module_file;
                   });

    // GUI JSON object
    nlohmann::json gui_info = {
        {"project", data.project},
        {"name", data.name + data.project_delimiter + data.project},
        {"structure", relative_adjustment + structure_file},
        {"stylesheet", relative_adjustment + data.stylesheet_file},
        {"modules", modules}};

    // Generate HTML
    std::string html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>" + data.project + "</title><script type=\"text/javascript\">const gui=" + gui_info.dump() + ";</script><script type=\"text/javascript\" src=\"/" + guis_js_path + "\"></script></head><body><noscript>Browser not supported: JavaScript required!</noscript></body></html>";

    /**
     * @brief Write contents to a file
     * @param filepath The location of the file to write to
     * @param contents The contents of the file to be written
     */
    std::function<void(std::string, std::string)> write_file = [](std::string const &filepath, std::string const &content)
    {
        std::filesystem::create_directories(std::filesystem::path(filepath).parent_path());
        std::ofstream file(filepath);
        if (!file)
            throw std::runtime_error("Failed to open file for writing: \"" + filepath + "\"");
        file << content;
    };

    // Write output files
    write_file((m_output_directory / data.html_file).string(), html);
    write_file((m_output_directory / structure_file).string(), structure);
}

std::filesystem::path generation_t::unique_filename(std::string const &extension)
{
    static std::mutex unique_filename_mutex;
    std::lock_guard<std::mutex> lock(unique_filename_mutex);

    static bool seeded = false;
    if (!seeded)
    {
        std::srand(static_cast<unsigned int>(std::time(0)));
        seeded = true;
    }
    while (true)
    {
        std::string const filename = std::to_string(std::rand()) + (extension.empty() ? "" : extension);
        bool unique = true;
        // Check against dependencies
        for (auto const &dependency : m_dependencies)
        {
            if (filename == dependency.second.filename())
            {
                unique = false;
                break;
            }
        }
        // Check against stylesheets
        for (auto const &gui : m_guis)
        {
            if (filename == std::filesystem::path(gui.stylesheet_file).filename())
            {
                unique = false;
                break;
            }
        }
        if (unique)
            return filename;
    }
}

void generation_t::build_all(bool const disallow_conflicts, bool const flatten_dependency_references, std::ostream const *debug_stream)
{
    std::filesystem::copy_options copy_options = std::filesystem::copy_options::recursive;
    if (disallow_conflicts)
    {
        copy_options |= std::filesystem::copy_options::overwrite_existing;
        // Check if output dependencies will conflict with generated files
        std::vector<std::filesystem::path> dependency_filenames = {};
        for (auto const &dependency : m_dependencies)
        {
            std::filesystem::path const dependency_source = dependency.first;
            std::filesystem::path const dependency_destination = dependency.second;
            dependency_filenames.push_back(dependency.first.filename());
            for (auto const &gui_data : m_guis)
            {
                for (auto const &generated_file : {gui_data.html_file, gui_data.structure_file})
                {
                    if (flatten_dependency_references)
                    {
                        if (generated_file != dependency_destination.filename())
                            continue;
                    }
                    else if (generated_file != dependency_destination)
                        continue;
                    throw std::runtime_error("A dependency of \"" + dependency_destination.string() + "\" will conflict with an automatically generated file");
                }
            }
            bool conflict = false;
            if (flatten_dependency_references)
                conflict = std::filesystem::exists(m_output_directory / dependency_destination.filename());
            else
                conflict = std::filesystem::exists(m_output_directory / dependency_destination);
            if (conflict)
                throw std::runtime_error("A dependency of \"" + dependency_source.string() + "\" will conflict with an already existing file");
        }
        if (flatten_dependency_references)
        {
            // Sort dependency filenames for faster finding...
            std::sort(dependency_filenames.begin(), dependency_filenames.end(), [](std::filesystem::path const &a, std::filesystem::path const &b)
                      { return a.filename().string() < b.filename().string(); });

            // Check if output dependencies will conflict with each other when filenames are flattened
            auto const duplicate = std::adjacent_find(dependency_filenames.begin(), dependency_filenames.end(), [](std::filesystem::path const &a, std::filesystem::path const &b)
                                                      { return a.filename().string() == b.filename().string(); });
            if (duplicate != dependency_filenames.end())
                throw std::runtime_error("Conflicting filename of \"" + duplicate->filename().string() + "\" between flattened dependencies");
        }
    }

    if (flatten_dependency_references)
    {
        // Flatten output stylesheet and module paths
        for (auto &&gui : m_guis)
        {
            gui.stylesheet_file = std::filesystem::path(gui.stylesheet_file).filename().string();
            for (auto &&module_file : gui.module_files)
                module_file = std::filesystem::path(module_file).filename().string();
        }
    }

    { // Remove sub dependencies
        std::vector<std::filesystem::path> dependencies_keys = {};
        for (auto const &dependency : m_dependencies)
            dependencies_keys.push_back(dependency.first);

        std::vector<std::filesystem::path> dependencies_allowed = remove_descendant_paths(dependencies_keys);
        std::vector<std::filesystem::path> dependencies_disallowed = {};
        for (auto const &dependency : m_dependencies)
        {
            if (std::find(dependencies_allowed.begin(), dependencies_allowed.end(), dependency.first) == dependencies_allowed.end())
                dependencies_disallowed.push_back(dependency.first);
        }
        for (auto const &disallowed_dependency : dependencies_disallowed)
            m_dependencies.erase(disallowed_dependency);
    }

    // Write GUI JavaScript file
    std::string guis_js_filename = unique_filename(".js");
    std::ofstream guis_js_stream(m_output_directory / guis_js_filename, std::ios::binary | std::ios::out);
    if (!guis_js_stream.is_open())
        throw std::runtime_error("Failed to create a file for writing output content to");
    guis_js_stream.write(reinterpret_cast<const char *>(sss_guis_js), sss_guis_js_len);
    guis_js_stream.close();

    std::mutex register_dependency_mutex;

    path_register_dependency_t const register_dependency = [&](std::filesystem::path const &file_path)
    {
        std::filesystem::path absolute_file_path = std::filesystem::absolute(file_path.lexically_normal());
        if (!std::filesystem::exists(absolute_file_path))
            throw std::runtime_error("No file(s) exists for dependency \"" + absolute_file_path.string() + "\"");

        std::lock_guard<std::mutex> lock(register_dependency_mutex);

        // If is already directly tracked
        if (m_dependencies.count(absolute_file_path))
        {
            std::filesystem::path const existing_dependency_path = m_dependencies.at(absolute_file_path);
            if (flatten_dependency_references)
                return existing_dependency_path.filename();
            else
                return existing_dependency_path;
        }
        // If is already indirectly tracked
        for (auto const &dependency : m_dependencies)
        {
            if (is_descendant(dependency.first, absolute_file_path))
            {
                auto const child_path = std::filesystem::proximate(absolute_file_path, dependency.first);
                if (flatten_dependency_references)
                    return (dependency.second.filename() / child_path);
                else
                    return (dependency.second / child_path);
            }
        }
        // Dependency is not currently being tracked
        std::filesystem::path dependency_file_path = unique_filename(absolute_file_path.extension());
        m_dependencies[absolute_file_path] = dependency_file_path;
        return dependency_file_path;
    };

    // Parallel processing loop
    std::vector<std::future<void>> futures = {};
    for (auto const &gui_data : m_guis)
        futures.push_back(std::async(std::launch::async, [&, gui_data, guis_js_filename, debug_stream]()
                                     { generate(gui_data, guis_js_filename, register_dependency, debug_stream); })); // Launch asynchronously

    // Wait for all threads to complete
    for (auto &future : futures)
        future.get(); // Blocks until the task completes and propagates exceptions

    // Copy dependencies
    for (auto const &dependency : m_dependencies)
    {
        std::filesystem::path const dependency_source = dependency.first;
        std::filesystem::path const dependency_destination = dependency.second;

        if (flatten_dependency_references)
            std::filesystem::copy((m_configuration_directory / dependency_source), (m_output_directory / dependency_destination.filename()), copy_options);
        else
        {
            std::filesystem::create_directories(m_output_directory / dependency_destination.parent_path());
            std::filesystem::copy((m_configuration_directory / dependency_source), (m_output_directory / dependency_destination), copy_options);
        }
    }
    return;
}
