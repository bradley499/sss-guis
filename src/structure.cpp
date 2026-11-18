#include "structure.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <unordered_set>

using namespace sss::guis;

namespace
{
    /**
     * @brief YAML tag for a file or directory to be registered as a dependency
     */
    std::string const FILE_TAG = "!file";

    /**
     * @brief Write output to debug output stream
     * @param debug_stream Pointer to a output stream (if nullptr then function will do nothing)
     * @param message Message to be written to debug output
     */
    void debug(std::ostream *debug_stream, std::string const &name, std::string const &message)
    {
        if (debug_stream == nullptr)
            return;
        static std::mutex debug_mutex;
        std::lock_guard<std::mutex> lock(debug_mutex);
        (*debug_stream) << name << ": " << message << std::endl;
        debug_stream->flush();
    }

    /**
     * @brief Converts a string to lowercase
     * @param string The string to convert
     * @returns Lowercase version of supplied string
     */
    inline std::string to_lower(std::string string)
    {
        std::transform(string.begin(), string.end(), string.begin(), [](unsigned char character)
                       { return std::tolower(character); });
        return string;
    }

    /**
     * @brief Validates whether a widget's name is value
     * @param name The name of the widget to validate
     * @throws When name is invalid
     */
    inline void validate_widget_name(std::string const &name)
    {
        if (name.empty())
            throw std::runtime_error("Failed to parse widget with no name defined");
        bool const name_contains_whitespace = std::any_of(name.begin(), name.end(), [](unsigned char character)
                                                          { return std::isspace(character); });
        if (name_contains_whitespace)
            throw std::runtime_error("Cannot name a widget `" + name + "` due to whitespace within its name");
        if (name == "null")
            throw std::runtime_error("Cannot name a widget `" + name + "` as that name is not allowed");
    }

    /**
     * @brief Validates whether a widget's type is value
     * @param type The type of the widget to validate
     * @throws When type is invalid
     */
    inline void validate_widget_type(std::string const &type)
    {
        if (type.empty())
            throw std::runtime_error("Failed to parse widget with no `type` defined");
        static constexpr std::string_view type_allowed_characters = "abcdefghijklmnopqrstuvwxyz0123456789._-";
        if (type.find_first_not_of(type_allowed_characters) != std::string_view::npos)
            throw std::runtime_error("Unable to declare a widget is of type `" + type + "` as it contains invalid characters");
    }

    /**
     * @brief Get the line number of a YAML Node object
     * @param node The YAML Node object to get the line number from
     * @return The line number of the YAML Node object
     */
    inline int yaml_node_line_number(YAML::Node const &node)
    {
        return (node.Mark().line + 1);
    }

    /**
     * @brief Counts the occurrence of widget types
     * @param widgets The widgets to count against
     * @returns Map of widget types and their occurrences
     */
    inline std::map<int, int> count_widget_type_occurrences(std::map<std::string, int> const &widgets)
    {
        std::map<int, int> type_counts;
        for (auto const &pair : widgets)
            type_counts[pair.second]++;
        return type_counts;
    }

    /**
     * @brief Convert a YAML node to a JSON object
     * @param yaml_node A YAML node
     * @returns JSON object
     */
    nlohmann::json yaml_to_json(YAML::Node const &yaml_node)
    {
        nlohmann::json json_value;
        if (yaml_node.IsScalar())
        {
            bool attempt_string = false;
            if (yaml_node.Tag() == FILE_TAG)
                goto YAML_TO_JSON_STRING;
            try
            {
                json_value = yaml_node.as<int>();
            }
            catch (YAML::BadConversion const &e)
            {
                try
                {
                    json_value = yaml_node.as<double>();
                }
                catch (YAML::BadConversion const &e)
                {
                    try
                    {
                        json_value = yaml_node.as<bool>();
                    }
                    catch (YAML::BadConversion const &e)
                    {
                        attempt_string = true;
                    }
                }
            }
            if (attempt_string)
            {
YAML_TO_JSON_STRING:
                try
                {
                    json_value = yaml_node.as<std::string>();
                }
                catch (YAML::BadConversion const &e)
                {
                    throw std::runtime_error("Failed to parse a YAML property");
                }
            }
        }
        else if (yaml_node.IsSequence())
        {
            for (auto const &item : yaml_node)
                json_value.push_back(yaml_to_json(item));
        }
        else if (yaml_node.IsMap())
        {
            for (auto it_yaml = yaml_node.begin(); it_yaml != yaml_node.end(); ++it_yaml)
            {
                std::string key;
                try
                {
                    key = it_yaml->first.as<std::string>();
                }
                catch (std::exception const &e)
                {
                    throw std::runtime_error("Failed to parse the key for a YAML property");
                }
                json_value[key] = yaml_to_json(it_yaml->second);
            }
        }
        return json_value;
    }
}

Structure::Structure(std::string const &name, std::ostream const *debug_stream)
    : m_widgets({}),
      m_widget_types({}),
      m_widget_contents({}),
      m_widget_contents_defaults({}),
      m_widget_files({}),
      m_parsed_files({}),
      m_name(name),
      m_debug_stream(const_cast<std::ostream *>(debug_stream))
{
}

Structure::~Structure()
{
    m_widgets.clear();
    m_widget_types.clear();
    m_widget_contents.clear();
    m_widget_files.clear();
    m_parsed_files.clear();
}

void Structure::populate_widget_defaults(WidgetContents const &widget_defaults)
{
    if (!widget_defaults.IsDefined())
        return;
    if (!widget_defaults.IsMap())
        throw std::runtime_error("Unable to parse widget defaults since a mappable structure is expected on line " + std::to_string(yaml_node_line_number(widget_defaults))); // Should never be thrown (expected to be caught prior to construction of `Structure` class)
    for (auto const &widget_default : widget_defaults)
    {
        WidgetType widget_type;
        try
        {
            widget_type = to_lower(widget_default.first.as<WidgetType>());
            validate_widget_type(widget_type);
        }
        catch (YAML::Exception const &e)
        {
            throw std::runtime_error("Failed to parse the `type` (string) of configured widget default on line " + std::to_string(yaml_node_line_number(widget_default)));
        }
        catch (std::runtime_error const &e)
        {
            throw std::runtime_error(std::string(e.what()) + " on line " + std::to_string(yaml_node_line_number(widget_default)));
        }
        YAML::Node const type = widget_default.second["type"];
        if (type.IsDefined())
        {
            if (!type.IsScalar() || (type.IsScalar() && type.as<WidgetType>() != widget_type))
                throw std::runtime_error("The widget default for the type `" + widget_type + "` has a conflicting yet redundant `type` definition - declared on line " + std::to_string(yaml_node_line_number(type)));
            else
                throw std::runtime_error("The widget default for the type `" + widget_type + "` has a redundant `type` definition - declared on line " + std::to_string(yaml_node_line_number(type)));
        }
        m_widget_contents_defaults.insert({widget_type, widget_default.second});
    }
}

void Structure::parse_file(std::filesystem::path const &configuration_file)
{
    std::vector<std::filesystem::path> files_to_parse({configuration_file});
    do
    {
        std::filesystem::path const current_file = files_to_parse.back();
        files_to_parse.pop_back();
        {
            auto const it = std::find(m_parsed_files.begin(), m_parsed_files.end(), current_file);
            if (it != m_parsed_files.end())
            {
                debug(m_debug_stream, m_name, "Additional reference was made to \"" + current_file.string() + "\"");
                continue;
            }
        }
        if (!std::filesystem::exists(current_file))
            throw std::runtime_error("Unable to find the configuration file of \"" + current_file.string() + "\"");
        debug(m_debug_stream, m_name, "Parsing configuration file \"" + current_file.string() + "\"...");
        try
        {
            std::vector<YAML::Node> documents = {};
            for (auto &document : YAML::LoadAllFromFile(current_file.string()))
            {
                if (!document.IsDefined() || !document.IsNull())
                    documents.push_back(document);
            }
            m_parsed_files.push_back(current_file);
            if (documents.empty())
            {
                debug(m_debug_stream, m_name, "The file \"" + current_file.string() + "\" is empty");
                continue;
            }
            for (auto &&contents : documents)
            {
                if (!contents.IsMap())
                    throw std::runtime_error("Unable to parse non-mappable structure on line " + std::to_string(yaml_node_line_number(contents)) + " of \"" + current_file.string() + "\"");

                for (auto const &widget_entry : contents)
                {
                    WidgetName widget_name;
                    try
                    {
                        widget_name = to_lower(widget_entry.first.as<WidgetName>());
                        validate_widget_name(widget_name);
                    }
                    catch (std::runtime_error const &e)
                    {
                        throw std::runtime_error(std::string(e.what()) + " on line " + std::to_string(yaml_node_line_number(widget_entry.first)) + " of \"" + current_file.string() + "\"");
                    }
                    catch (std::exception const &e)
                    {
                        throw std::runtime_error("Failed to parse the name (string) of a widget on line " + std::to_string(yaml_node_line_number(widget_entry.first)) + " of \"" + current_file.string() + "\"");
                    }

                    if (widget_name == "include")
                        continue;

                    YAML::Node const type = widget_entry.second["type"];
                    if (!type.IsDefined())
                        throw std::runtime_error("The widget `" + widget_name + "` has no `type` definition - widget declared on line " + std::to_string(yaml_node_line_number(widget_entry.first)) + " of \"" + current_file.string() + "\"");

                    WidgetType widget_type;
                    try
                    {
                        widget_type = to_lower(type.as<WidgetType>());
                        validate_widget_type(widget_type);
                    }
                    catch (YAML::Exception const &e)
                    {
                        throw std::runtime_error("Failed to parse the `type` (string) of widget `" + widget_name + "` on line " + std::to_string(yaml_node_line_number(type)) + " of \"" + current_file.string() + "\"");
                    }
                    catch (std::runtime_error const &e)
                    {
                        throw std::runtime_error(std::string(e.what()) + " on line " + std::to_string(yaml_node_line_number(type)) + " of \"" + current_file.string() + "\"");
                    }

                    // Create a mutable copy of the node to remove the "type" key
                    WidgetContents widget_content_node = widget_entry.second;
                    widget_content_node.remove("type");
                    try
                    {
                        add_widget(widget_name, widget_type, widget_entry.first, widget_content_node, static_cast<WidgetFileReference>(m_parsed_files.size() - 1));
                    }
                    catch (std::runtime_error const &e)
                    {
                        throw std::runtime_error(std::string(e.what()) + " of \"" + current_file.string() + "\"");
                    }
                }

                YAML::Node const include_node = contents["include"];
                if (include_node.IsDefined())
                {
                    auto const process_include_entry = [&](YAML::Node const &include_entry)
                    {
                        if (!include_entry.IsScalar())
                            throw std::runtime_error("Expected a string path for an include on line " + std::to_string(yaml_node_line_number(include_entry)) + " of \"" + current_file.string() + "\"");

                        std::filesystem::path include_relative_path;
                        try
                        {
                            include_relative_path = include_entry.as<std::string>();
                        }
                        catch (std::exception const &e)
                        {
                            throw std::runtime_error("Expected a string path for an include on line " + std::to_string(yaml_node_line_number(include_entry)) + " of \"" + current_file.string() + "\"");
                        }

                        if (include_relative_path.empty())
                            throw std::runtime_error("Expected a string path for an include on line " + std::to_string(yaml_node_line_number(include_entry)) + " of \"" + current_file.string() + "\"");

                        // The 'current_file' is the absolute path of the current YAML file being parsed.
                        // We want to resolve 'include_relative_path' relative to the directory of 'current_file'.
                        std::filesystem::path const parent_directory = std::filesystem::absolute(current_file).parent_path();

                        // Construct the absolute path of the included file
                        std::filesystem::path resolved_include_path;
                        if (include_relative_path.is_absolute())
                            resolved_include_path = include_relative_path;
                        else
                            resolved_include_path = parent_directory / include_relative_path;

                        files_to_parse.push_back(std::filesystem::absolute(resolved_include_path.lexically_normal()).string());
                    };

                    if (include_node.IsSequence())
                    {
                        for (YAML::Node const &include_entry : include_node)
                            process_include_entry(include_entry);
                    }
                    else if (include_node.IsScalar())
                        process_include_entry(include_node);
                    else if (include_node.Type() != YAML::NodeType::Null)
                        throw std::runtime_error("Unable to parse `include` since a string or list is expected on line " + std::to_string(yaml_node_line_number(include_node)) + " of \"" + current_file.string() + "\"");
                }
            }
        }
        catch (YAML::Exception const &e)
        {
            std::string yaml_error = e.msg;
            if (yaml_error.empty())
                yaml_error = "Unknown YAML parsing error occurred";
            else // Reformat the YAML error messages to be more inline with other error messages
            {
                /**
                 * @brief Changes the text within an error message
                 * @param error The error message to modify
                 * @param replace The text to replace
                 * @param replacement The text to replace with
                 * @returns A modified error message
                 */
                auto const change_yaml_error = [](std::string error, const std::string &replace, const std::string &replacement)
                {
                    size_t position = 0;
                    while ((position = error.find(replace, position)) != std::string::npos)
                    {
                        error.replace(position, replace.length(), replacement);
                        position += replacement.length();
                    }
                    return error;
                };
                if (yaml_error.compare(0, strlen(YAML::ErrorMsg::YAML_VERSION), YAML::ErrorMsg::YAML_VERSION) == 0)
                {
                    yaml_error = change_yaml_error(yaml_error, YAML::ErrorMsg::YAML_VERSION, "Bad YAML version `") + "`";
                }
                else if (yaml_error.compare(0, strlen(YAML::ErrorMsg::INVALID_UNICODE), YAML::ErrorMsg::INVALID_UNICODE) == 0)
                {
                    yaml_error = change_yaml_error(yaml_error, YAML::ErrorMsg::INVALID_UNICODE, "Invalid unicode `") + "`";
                }
                else if (yaml_error.compare(0, strlen(YAML::ErrorMsg::INVALID_ESCAPE), YAML::ErrorMsg::INVALID_ESCAPE) == 0)
                {
                    yaml_error = change_yaml_error(yaml_error, YAML::ErrorMsg::INVALID_ESCAPE, "Unknown escape character `") + "`";
                }
                else if (yaml_error.compare(0, strlen(YAML::ErrorMsg::UNKNOWN_ANCHOR), YAML::ErrorMsg::UNKNOWN_ANCHOR) == 0)
                {
                    yaml_error = change_yaml_error(yaml_error, YAML::ErrorMsg::UNKNOWN_ANCHOR, "The referenced anchor `") + "` is not defined";
                }
                else // Static error message (no content inserted from YAML file)
                {
                    yaml_error = change_yaml_error(yaml_error, ",", "");
                    yaml_error = change_yaml_error(yaml_error, ";", "");
                    yaml_error = change_yaml_error(yaml_error, "*including*", "including");
                    yaml_error = change_yaml_error(yaml_error, "EOF", "End Of File character");
                    yaml_error = change_yaml_error(yaml_error, "can't", "cannot");
                    yaml_error = change_yaml_error(yaml_error, "nodes", "properties");
                    yaml_error = change_yaml_error(yaml_error, "node", "property");
                    yaml_error.at(0) = std::toupper(static_cast<unsigned char>(yaml_error.at(0)));
                }
            }
            throw std::runtime_error(yaml_error + " on line " + std::to_string(e.mark.line + 1) + " of \"" + current_file.string() + "\"");
        }
        catch (std::runtime_error const &e)
        {
            throw e;
        }
        catch (std::exception const &e)
        {
            throw std::runtime_error("Unable to include an unparseable file of \"" + current_file.string() + "\"");
        }
    } while (!files_to_parse.empty());
}

void Structure::add_widget(WidgetName const &name, WidgetType const &type, YAML::Node const &widget_base_node, WidgetContents const &contents, WidgetFileReference const file_reference)
{
    WidgetTypeIdentifier type_id = -1;
    auto const it = std::find(m_widget_types.begin(), m_widget_types.end(), type);
    if (it != m_widget_types.end())
        type_id = std::distance(m_widget_types.begin(), it);
    else
    {
        m_widget_types.push_back(type);
        type_id = (m_widget_types.size() - 1);
    }

    if (m_widgets.count(name))
        throw std::runtime_error("Duplicate definitions of the widget `" + name + "` on line " + std::to_string(yaml_node_line_number(widget_base_node)));
    if (m_widget_contents_defaults.count(type))
    {
        YAML::Node generated_output = YAML::Clone(m_widget_contents_defaults.at(type));
        struct NodeComparison
        {
            YAML::Node generated_node;
            YAML::Node defined_node;
        };

        std::vector<NodeComparison> processing_stack = {};
        processing_stack.push_back({generated_output, contents});

        while (!processing_stack.empty())
        {
            NodeComparison task = processing_stack.back();
            processing_stack.pop_back();

            YAML::Node &generated_node = task.generated_node;
            YAML::Node const &defined_node = task.defined_node;

            if (generated_node.IsDefined() && !generated_node.IsNull() && generated_node.Type() != defined_node.Type() && !defined_node.IsNull())
                throw std::runtime_error("Structural conflict between widget `defaults` for the widget `" + name + "` of type `" + type + "` on line " + std::to_string(yaml_node_line_number(defined_node)));

            if (defined_node.IsMap())
            {
                for (auto it = defined_node.begin(); it != defined_node.end(); ++it)
                {
                    std::string const key = it->first.as<std::string>();
                    YAML::Node const source_value = it->second;
                    if (generated_node[key])
                        processing_stack.push_back({generated_node[key], source_value});
                    else
                        generated_node[key] = YAML::Clone(source_value);
                }
            }
            else
            {
                if (!defined_node.IsNull())
                    generated_node = YAML::Clone(defined_node); // Override default
            }
        }
        m_widget_contents[name] = generated_output;
    }
    else
        m_widget_contents[name] = contents;
    m_widgets[name] = type_id;
    m_widget_files[name] = file_reference;
}

void Structure::validate_and_prune_hierarchy(PathRegisterDependency const &path_register_dependency_callback)
{
    std::unordered_set<WidgetName> referenced_widgets = {};

    // We store the current widget and the path of ancestors taken to reach it
    struct Widget
    {
        WidgetName name;
        std::vector<WidgetName> hierarchy;
    };
    std::vector<Widget> widget_hierarchies_to_traverse = {};

    if (m_widgets.count("main"))
    {
        widget_hierarchies_to_traverse.push_back({"main", {}});
        referenced_widgets.insert("main"); // Expect no references to `main` object
    }

    /**
     * @brief Iteratively validate object references and prevent recursive structures
     * @param widget The current widget to search
     * @param widget_yaml_contents The YAML content of the widget
     * @param widget_file The source file of the widget
     */
    auto const validate_references_iterative = [&](Widget const &widget, YAML::Node const &widget_yaml_contents, std::filesystem::path const &widget_file)
    {
        std::vector<YAML::Node> widget_contents = {};
        widget_contents.push_back(widget_yaml_contents);

        while (!widget_contents.empty())
        {
            YAML::Node contents = widget_contents.back();
            widget_contents.pop_back();

            if (contents.IsSequence())
            {
                for (auto const &content : contents)
                    widget_contents.push_back(content);
            }
            else if (contents.IsMap())
            {
                for (auto it = contents.begin(); it != contents.end(); ++it)
                {
                    std::string key;
                    try
                    {
                        key = it->first.as<std::string>();
                    }
                    catch (std::exception const &e)
                    {
                        throw std::runtime_error("Failed to parse the key for a YAML property of `" + widget.name + "` on line " + std::to_string(yaml_node_line_number(it->first)) + " of \"" + widget_file.string() + "\"");
                    }
                    YAML::Node value = it->second;
                    if (key == "object")
                    {
                        if (value.IsNull())
                            throw std::runtime_error("No child object reference has been defined for `object` of `" + widget.name + "` on line " + std::to_string(yaml_node_line_number(it->first)) + " of \"" + widget_file.string() + "\"");
                        WidgetName object_name;
                        try
                        {
                            if (value.IsScalar())
                            {
                                object_name = to_lower(value.as<WidgetName>());
                                if (object_name.empty())
                                    throw YAML::BadConversion(value.Mark());
                            }
                            else
                                throw YAML::BadConversion(value.Mark());
                        }
                        catch (YAML::BadConversion const &e)
                        {
                            throw std::runtime_error("Child object reference of `" + widget.name + "` refers to a non-descriptive `object` on line " + std::to_string(yaml_node_line_number(value)) + " of \"" + widget_file.string() + "\"");
                        }
                        if (m_widgets.count(object_name) == 0)
                            throw std::runtime_error("Child object reference from `" + widget.name + "` to `" + object_name + "` does not relate to any known widgets - on line " + std::to_string(yaml_node_line_number(value)) + " of \"" + widget_file.string() + "\"");
                        WidgetType const tag = value.Tag();
                        if (tag.size() > 1)
                        {
                            WidgetType const object_type = m_widget_types.at(m_widgets.at(object_name));
                            WidgetType const object_type_tag = to_lower(tag.substr(1)); // Removes the tag handle
                            if (object_type != object_type_tag)
                                throw std::runtime_error("Child object reference from `" + widget.name + "` to `" + object_name + "` is of type `" + object_type + "` which does not match the enforced type of `" + object_type_tag + "` on line " + std::to_string(yaml_node_line_number(value)) + " of \"" + widget_file.string() + "\"");
                        }
                        if (object_name == widget.name || std::find(widget.hierarchy.begin(), widget.hierarchy.end(), object_name) != widget.hierarchy.end())
                        {
                            std::string const trace_next = " > ";
                            std::string const trace_recursive_object = " (recursive object)";
                            std::string trace = "";
                            bool found_first_reference = false;
                            for (WidgetType const &widget_ancestor : widget.hierarchy)
                            {
                                if (widget_ancestor == object_name)
                                    found_first_reference = true;
                                trace += "`" + widget_ancestor + "`" + ((widget_ancestor == object_name) ? trace_recursive_object : "") + trace_next;
                            }
                            trace += "`" + widget.name + "`" + ((!found_first_reference && (widget.name == object_name)) ? trace_recursive_object : "") + trace_next + "`" + object_name + "` (recursive reference)";
                            throw std::runtime_error("Recursive object reference detected for `" + object_name + "` [" + trace + "] on line " + std::to_string(yaml_node_line_number(value)) + " of \"" + widget_file.string() + "\"");
                        }
                        if (referenced_widgets.find(object_name) == referenced_widgets.end())
                        {
                            referenced_widgets.insert(object_name);
                            std::vector<WidgetName> next_widget_hierarchy = widget.hierarchy;
                            next_widget_hierarchy.push_back(widget.name);
                            widget_hierarchies_to_traverse.push_back({object_name, next_widget_hierarchy});
                        }
                    }
                    else
                    {
                        if (value.Tag() == FILE_TAG)
                        {
                            try
                            {
                                if (value.IsScalar())
                                {
                                    std::string const file_path_string = value.as<WidgetName>();
                                    if (file_path_string.empty())
                                        throw YAML::BadConversion(value.Mark());
                                    std::filesystem::path file_path = std::filesystem::path(file_path_string);
                                    if (file_path.is_relative())
                                        file_path = widget_file.parent_path() / file_path;
                                    std::string const dependency_file_path_string = path_register_dependency_callback(file_path);
                                    std::string const dependency_file_name_string = std::filesystem::path(dependency_file_path_string).filename().string();
                                    debug(m_debug_stream, m_name, "Adding referenced " + std::string(std::filesystem::is_directory(file_path) ? "directory" : "file") + " \"" + file_path.lexically_normal().string() + "\" as a dependency" + ((dependency_file_name_string != file_path.filename().string()) ? " named \"" + std::filesystem::path(dependency_file_path_string).filename().string() + "\"" : ""));
                                    value = dependency_file_path_string;
                                }
                                else
                                    throw YAML::BadConversion(value.Mark());
                            }
                            catch (YAML::BadConversion const &e)
                            {
                                throw std::runtime_error("Unable to add an (non-string) referenced dependency on line " + std::to_string(yaml_node_line_number(value)) + " of \"" + widget_file.string() + "\"");
                            }
                            catch (std::runtime_error const &e)
                            {
                                throw std::runtime_error(std::string(e.what()) + " on line " + std::to_string(yaml_node_line_number(value)) + " of \"" + widget_file.string() + "\"");
                            }
                        }
                        else if (value.IsMap() || value.IsSequence())
                            widget_contents.push_back(value);
                    }
                }
            }
        }
    };

    // Iteratively find references throughout the hierarchy of used widgets
    while (!widget_hierarchies_to_traverse.empty())
    {
        Widget current_widget = widget_hierarchies_to_traverse.back();
        widget_hierarchies_to_traverse.pop_back();
        if (m_widget_contents.count(current_widget.name))
            validate_references_iterative(current_widget, m_widget_contents.at(current_widget.name), m_parsed_files.at(m_widget_files.at(current_widget.name)));
    }

    // Identify orphaned widgets
    std::vector<WidgetName> widgets_to_remove = {};
    for (auto const &pair : m_widgets)
    {
        if (referenced_widgets.find(pair.first) == referenced_widgets.end())
            widgets_to_remove.push_back(pair.first);
    }

    if (widgets_to_remove.size() > 0)
        debug(m_debug_stream, m_name, "Pruning " + std::to_string(widgets_to_remove.size()) + " widget" + (widgets_to_remove.size() != 1 ? "s" : "") + "...");
    bool const pruned_all_widgets = (widgets_to_remove.size() == m_widgets.size());

    // Prune orphaned widgets
    for (WidgetName const &name : widgets_to_remove)
    {
        m_widgets.erase(name);
        m_widget_contents.erase(name);
        debug(m_debug_stream, m_name, "Pruned `" + name + "`");
    }
    widgets_to_remove.clear();

    if (pruned_all_widgets)
    {
        m_widget_types.clear();
        return;
    }

    // Create a mapping from old type_id to new type_id
    using WidgetTypeIdentifierOccurrences = int;
    std::map<WidgetTypeIdentifier, WidgetTypeIdentifierOccurrences> const type_occurrences = count_widget_type_occurrences(m_widgets);
    std::vector<WidgetTypeIdentifier> old_to_new_type_id_map(m_widget_types.size(), -1);
    std::vector<WidgetType> new_widget_types = {};
    WidgetTypeIdentifier current_new_type_identifier = 0;

    for (WidgetTypeIdentifier existing_type_identifier = 0; existing_type_identifier < static_cast<WidgetTypeIdentifier>(m_widget_types.size()); ++existing_type_identifier)
    {
        if (type_occurrences.count(existing_type_identifier) && type_occurrences.at(existing_type_identifier) > 0)
        {
            old_to_new_type_id_map.at(existing_type_identifier) = current_new_type_identifier;
            new_widget_types.push_back(std::move(m_widget_types[existing_type_identifier]));
            current_new_type_identifier++;
        }
    }

    for (auto &pair : m_widgets)
        pair.second = old_to_new_type_id_map.at(pair.second);

    m_widget_types = std::move(new_widget_types);
}

void Structure::number_references()
{
    if (m_widgets.empty())
        return;
    debug(m_debug_stream, m_name, "Updating references of objects for numeric positioning...");
    size_t object_reference_identifier = 0;
    for (auto &pair : m_widget_contents)
    {
        WidgetName const &widget_name = pair.first;
        std::filesystem::path const &widget_file = m_parsed_files.at(m_widget_files.at(widget_name));
        debug(m_debug_stream, m_name, "Resolving references to object `" + widget_name + "` with index: " + std::to_string(object_reference_identifier++));
        std::vector<YAML::Node> nested_properties;
        nested_properties.push_back(pair.second);
        while (!nested_properties.empty())
        {
            YAML::Node property = nested_properties.back();
            nested_properties.pop_back();
            if (property.IsSequence())
            {
                for (auto const &nested_property : property)
                    nested_properties.push_back(nested_property);
            }
            else if (property.IsMap())
            {
                for (auto const &nested_property : property)
                {
                    std::string const key = nested_property.first.as<std::string>();
                    YAML::Node const value = nested_property.second;
                    if (key == "object")
                    {
                        if (value.IsNull())
                            throw std::runtime_error("No child object reference has been defined for `object` of `" + widget_name + "` on line " + std::to_string(yaml_node_line_number(nested_property.first)) + " of \"" + widget_file.string() + "\""); // Should never be thrown (expected to be caught prior in `validate_and_prune_hierarchy` method)
                        WidgetName object_name;
                        try
                        {
                            if (value.IsScalar())
                            {
                                object_name = to_lower(value.as<WidgetName>());
                                if (object_name.empty())
                                    throw YAML::BadConversion(value.Mark());
                            }
                            else
                                throw YAML::BadConversion(value.Mark());
                        }
                        catch (YAML::BadConversion const &e)
                        {
                            throw std::runtime_error("Child object reference of `" + widget_name + "` refers to a non-descriptive `object` on line " + std::to_string(yaml_node_line_number(value)) + " of \"" + widget_file.string() + "\""); // Should never be thrown (expected to be caught prior in `validate_and_prune_hierarchy` method)
                        }
                        auto const widget_it = m_widgets.find(object_name);
                        if (widget_it != m_widgets.end())
                            property[key] = std::distance(m_widgets.begin(), widget_it);
                        else
                            throw std::runtime_error("An unexpected error occurred whilst handling a dangling child object reference for `" + object_name + "` of `" + widget_name + "` on line " + std::to_string(yaml_node_line_number(value)) + " of \"" + widget_file.string() + "\""); // Should never be thrown (expected to be caught prior in `validate_and_prune_hierarchy` method)
                    }
                    else if (value.IsMap() || value.IsSequence())
                        nested_properties.push_back(value);
                }
            }
        }
    }
}

std::string Structure::build(PathRegisterDependency const &path_register_dependency_callback, bool const numeric_references)
{
    validate_and_prune_hierarchy(path_register_dependency_callback);
    if (numeric_references)
        number_references();

    int main = -1;
    auto const it = m_widgets.find("main");
    if (it != m_widgets.end())
    {
        if (numeric_references)
            main = std::distance(m_widgets.begin(), it); // Get index of 'main' widget
    }
    else
        throw std::runtime_error("No `main` widget was found!");

    nlohmann::json output_json;
    if (!numeric_references)
        output_json["main"] = WidgetName("main");
    else
        output_json["main"] = main;
    if (numeric_references)
    {
        nlohmann::json widgets_array = nlohmann::json::array();
        for (auto const &pair : m_widgets)
        {
            nlohmann::json widget_entry = nlohmann::json::array();
            widget_entry.push_back(pair.second);
            widget_entry.push_back(yaml_to_json(m_widget_contents[pair.first]));
            widgets_array.push_back(widget_entry);
        }
        output_json["widgets"] = widgets_array;
    }
    else
    {
        nlohmann::json widgets_array = nlohmann::json::object();
        for (auto const &pair : m_widgets)
        {
            nlohmann::json widget_entry = nlohmann::json::array();
            widget_entry.push_back(pair.second);
            widget_entry.push_back(yaml_to_json(m_widget_contents[pair.first]));
            widgets_array[pair.first] = widget_entry;
        }
        output_json["widgets"] = widgets_array;
    }
    output_json["types"] = m_widget_types;
    return output_json.dump();
}
