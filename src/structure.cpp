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

structure_t::structure_t(std::string const &file, std::string const &name, std::ostream const *debug_stream)
    : m_widgets({}),
      m_widget_types({}),
      m_widget_contents({}),
      m_widget_files({}),
      m_parsed_files({}),
      m_name(name),
      m_debug_stream(const_cast<std::ostream *>(debug_stream))
{
    parse_file(std::filesystem::absolute(file).lexically_normal());
}

structure_t::~structure_t()
{
    m_widgets.clear();
    m_widget_types.clear();
    m_widget_contents.clear();
    m_widget_files.clear();
    m_parsed_files.clear();
}

void structure_t::parse_file(std::filesystem::path const &file)
{
    std::vector<std::filesystem::path> dependency_files({file});
    do
    {
        std::filesystem::path const dependency_file = dependency_files.back();
        dependency_files.pop_back();
        {
            auto const it = std::find(m_parsed_files.begin(), m_parsed_files.end(), dependency_file);
            if (it != m_parsed_files.end())
            {
                debug(m_debug_stream, m_name, "Additional reference was made to \"" + dependency_file.string() + "\"");
                continue;
            }
        }
        if (!std::filesystem::exists(dependency_file))
            throw std::runtime_error("Unable to find dependency file of \"" + dependency_file.string() + "\"");
        debug(m_debug_stream, m_name, "Parsing configuration dependency \"" + dependency_file.string() + "\"...");
        try
        {
            std::vector<YAML::Node> documents = {};
            for (auto &document : YAML::LoadAllFromFile(dependency_file.string()))
            {
                if (!document.IsDefined() || !document.IsNull())
                    documents.push_back(document);
            }
            m_parsed_files.push_back(dependency_file);
            if (documents.empty())
            {
                debug(m_debug_stream, m_name, "Empty dependency file located at \"" + dependency_file.string() + "\"");
                continue;
            }
            for (auto &&contents : documents)
            {
                if (!contents.IsMap())
                    throw std::runtime_error("Unable to parse non-mappable structure on line " + std::to_string(yaml_node_line_number(contents)) + " of \"" + dependency_file.string() + "\"");

                for (auto const &widget_entry : contents)
                {
                    widget_name_t widget_name;
                    try
                    {
                        widget_name = to_lower(widget_entry.first.as<widget_name_t>());
                        validate_widget_name(widget_name);
                    }
                    catch (std::runtime_error const &e)
                    {
                        throw std::runtime_error(std::string(e.what()) + " on line " + std::to_string(yaml_node_line_number(widget_entry.first)) + " of \"" + dependency_file.string() + "\"");
                    }
                    catch (std::exception const &e)
                    {
                        throw std::runtime_error("Failed to parse the name (string) of a widget on line " + std::to_string(yaml_node_line_number(widget_entry.first)) + " of \"" + dependency_file.string() + "\"");
                    }

                    if (widget_name == "dependencies")
                        continue;

                    YAML::Node const type = widget_entry.second["type"];
                    if (!type.IsDefined())
                        throw std::runtime_error("The widget `" + widget_name + "` has no `type` definition - widget declared on line " + std::to_string(yaml_node_line_number(widget_entry.first)) + " of \"" + dependency_file.string() + "\"");

                    widget_type_t widget_type;
                    try
                    {
                        widget_type = to_lower(type.as<widget_type_t>());
                        validate_widget_type(widget_type);
                    }
                    catch (YAML::Exception const &e)
                    {
                        throw std::runtime_error("Failed to parse the `type` (string) of widget `" + widget_name + "` on line " + std::to_string(yaml_node_line_number(type)) + " of \"" + dependency_file.string() + "\"");
                    }
                    catch (std::runtime_error const &e)
                    {
                        throw std::runtime_error(std::string(e.what()) + " on line " + std::to_string(yaml_node_line_number(type)) + " of \"" + dependency_file.string() + "\"");
                    }

                    // Create a mutable copy of the node to remove the "type" key
                    widget_contents_t widget_content_node = widget_entry.second;
                    widget_content_node.remove("type");
                    try
                    {
                        add_widget(widget_name, widget_type, widget_content_node, static_cast<widget_file_reference_t>(m_parsed_files.size() - 1));
                    }
                    catch (std::runtime_error const &e)
                    {
                        throw std::runtime_error(std::string(e.what()) + " on line " + std::to_string(yaml_node_line_number(widget_entry.first)) + " of \"" + dependency_file.string() + "\"");
                    }
                }

                YAML::Node const dependencies = contents["dependencies"];
                if (dependencies.IsDefined())
                {
                    if (dependencies.IsSequence())
                    {
                        for (YAML::Node const &dependency : dependencies)
                        {
                            if (!dependency.IsScalar())
                                throw std::runtime_error("Expected a string path for a dependency on line " + std::to_string(yaml_node_line_number(dependency)) + " of \"" + dependency_file.string() + "\"");
                            std::filesystem::path dependency_relative_path;
                            try
                            {
                                dependency_relative_path = dependency.as<std::string>();
                            }
                            catch (std::exception const &e)
                            {
                                throw std::runtime_error("Expected a string path for a dependency on line " + std::to_string(yaml_node_line_number(dependency)) + " of \"" + dependency_file.string() + "\"");
                            }

                            // The 'dependency_file' is the absolute path of the current YAML file being parsed.
                            // We want to resolve 'dependency_relative_path' relative to the directory of 'dependency_file'.
                            std::filesystem::path parent_dir = std::filesystem::absolute(dependency_file).parent_path();

                            // Construct the absolute path of the dependency
                            std::filesystem::path resolved_dependency_path;
                            if (dependency_relative_path.is_absolute())
                                resolved_dependency_path = dependency_relative_path;
                            else
                                resolved_dependency_path = parent_dir / dependency_relative_path;

                            dependency_files.push_back(std::filesystem::absolute(resolved_dependency_path.lexically_normal()).string());
                        }
                    }
                    else if (dependencies.Type() != YAML::NodeType::Null)
                        throw std::runtime_error("Unable to parse `dependencies` since a list is expected on line " + std::to_string(yaml_node_line_number(dependencies)) + " of \"" + dependency_file.string() + "\"");
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
            throw std::runtime_error(yaml_error + " on line " + std::to_string(e.mark.line + 1) + " of \"" + dependency_file.string() + "\"");
        }
        catch (std::runtime_error const &e)
        {
            throw e;
        }
        catch (std::exception const &e)
        {
            throw std::runtime_error("Unable to parse dependency file of \"" + dependency_file.string() + "\"");
        }
    } while (!dependency_files.empty());
}

void structure_t::add_widget(widget_name_t const &name, widget_type_t const &type, widget_contents_t const &contents, widget_file_reference_t const file_reference)
{
    int type_id = -1;
    auto const it = std::find(m_widget_types.begin(), m_widget_types.end(), type);
    if (it != m_widget_types.end())
        type_id = std::distance(m_widget_types.begin(), it);
    else
    {
        m_widget_types.push_back(type);
        type_id = m_widget_types.size() - 1;
    }

    if (m_widgets.count(name))
        throw std::runtime_error("Duplicate definitions of the widget `" + name + "`");
    m_widgets[name] = type_id;
    m_widget_contents[name] = contents;
    m_widget_files[name] = file_reference;
}

void structure_t::validate_and_prune_hierarchy()
{
    std::unordered_set<widget_name_t> referenced_widgets = {};

    // We store the current widget and the path of ancestors taken to reach it
    struct widget_t
    {
        widget_name_t name;
        std::vector<widget_name_t> hierarchy;
    };
    std::vector<widget_t> widget_hierarchies_to_traverse = {};

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
    auto const validate_references_iterative = [&](widget_t const &widget, YAML::Node const &widget_yaml_contents, std::filesystem::path const &widget_file)
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
                        widget_name_t object_name;
                        try
                        {
                            if (value.IsScalar())
                            {
                                object_name = to_lower(value.as<widget_name_t>());
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
                        widget_type_t const tag = value.Tag();
                        switch (tag.size())
                        {
                        case 0:
                            break;
                        case 1:
                            if (tag != "?")
                                throw std::runtime_error("Unexpected `" + tag + "` character prior to child object reference from `" + widget.name + "` to `" + object_name + "` on line " + std::to_string(yaml_node_line_number(value)) + " of \"" + widget_file.string() + "\"");
                            break;
                        default:
                        {
                            widget_type_t const object_type = m_widget_types.at(m_widgets.at(object_name));
                            widget_type_t const object_type_tag = to_lower(tag.substr(1)); // Removes the tag handle
                            if (object_type != object_type_tag)
                                throw std::runtime_error("Child object reference from `" + widget.name + "` to `" + object_name + "` is of type `" + object_type + "` which does not match the enforced type of `" + object_type_tag + "` on line " + std::to_string(yaml_node_line_number(value)) + " of \"" + widget_file.string() + "\"");
                            break;
                        }
                        }
                        if (object_name == widget.name || std::find(widget.hierarchy.begin(), widget.hierarchy.end(), object_name) != widget.hierarchy.end())
                        {
                            std::string const trace_next = " > ";
                            std::string const trace_recursive_object = " (recursive object)";
                            std::string trace = "";
                            bool found_first_reference = false;
                            for (widget_type_t const &widget_ancestor : widget.hierarchy)
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
                            std::vector<widget_name_t> next_widget_hierarchy = widget.hierarchy;
                            next_widget_hierarchy.push_back(widget.name);
                            widget_hierarchies_to_traverse.push_back({object_name, next_widget_hierarchy});
                        }
                    }
                    else if (value.IsMap() || value.IsSequence())
                        widget_contents.push_back(value);
                }
            }
        }
    };

    // Iteratively find references throughout the hierarchy of used widgets
    while (!widget_hierarchies_to_traverse.empty())
    {
        widget_t current_widget = widget_hierarchies_to_traverse.back();
        widget_hierarchies_to_traverse.pop_back();
        if (m_widget_contents.count(current_widget.name))
            validate_references_iterative(current_widget, m_widget_contents.at(current_widget.name), m_parsed_files.at(m_widget_files.at(current_widget.name)));
    }

    // Identify orphaned widgets
    std::vector<widget_name_t> widgets_to_remove = {};
    for (auto const &pair : m_widgets)
    {
        if (referenced_widgets.find(pair.first) == referenced_widgets.end())
            widgets_to_remove.push_back(pair.first);
    }

    if (widgets_to_remove.size() > 0)
        debug(m_debug_stream, m_name, "Pruning " + std::to_string(widgets_to_remove.size()) + " widget" + (widgets_to_remove.size() != 1 ? "s" : "") + "...");
    bool const pruned_all_widgets = (widgets_to_remove.size() == m_widgets.size());

    // Prune orphaned widgets
    for (widget_name_t const &name : widgets_to_remove)
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
    using widget_type_identifier_occurrences_t = int;
    std::map<widget_type_identifier_t, widget_type_identifier_occurrences_t> const type_occurrences = count_widget_type_occurrences(m_widgets);
    std::vector<widget_type_identifier_t> old_to_new_type_id_map(m_widget_types.size(), -1);
    std::vector<widget_type_t> new_widget_types = {};
    widget_type_identifier_t current_new_type_identifier = 0;

    for (std::size_t i = 0; i < m_widget_types.size(); ++i)
    {
        if (type_occurrences.at(i) > 0)
        {
            old_to_new_type_id_map[i] = current_new_type_identifier;
            new_widget_types.push_back(std::move(m_widget_types[i]));
            current_new_type_identifier++;
        }
    }

    for (auto &pair : m_widgets)
        pair.second = old_to_new_type_id_map[pair.second];

    m_widget_types = std::move(new_widget_types);
}

void structure_t::number_references()
{
    if (m_widgets.empty())
        return;
    debug(m_debug_stream, m_name, "Updating references of objects for numeric positioning...");
    size_t object_reference_identifier = 0;
    for (auto &pair : m_widget_contents)
    {
        widget_name_t const &widget_name = pair.first;
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
                            throw std::runtime_error("No child object reference has been defined for `object` of `" + widget_name + "` on line " + std::to_string(yaml_node_line_number(nested_property.first)) + " of \"" + widget_file.string() + "\""); // Should never be thrown (expected to be caught prior in `prune_references` method)
                        widget_name_t object_name;
                        try
                        {
                            if (value.IsScalar())
                            {
                                object_name = to_lower(value.as<widget_name_t>());
                                if (object_name.empty())
                                    throw YAML::BadConversion(value.Mark());
                            }
                            else
                                throw YAML::BadConversion(value.Mark());
                        }
                        catch (YAML::BadConversion const &e)
                        {
                            throw std::runtime_error("Child object reference of `" + widget_name + "` refers to a non-descriptive `object` on line " + std::to_string(yaml_node_line_number(value)) + " of \"" + widget_file.string() + "\""); // Should never be thrown (expected to be caught prior in `prune_references` method)
                        }
                        auto const widget_it = m_widgets.find(object_name);
                        if (widget_it != m_widgets.end())
                            property[key] = std::distance(m_widgets.begin(), widget_it);
                        else
                            throw std::runtime_error("An unexpected error occurred whilst handling a dangling child object reference for `" + object_name + "` of `" + widget_name + "` on line " + std::to_string(yaml_node_line_number(value)) + " of \"" + widget_file.string() + "\""); // Should never be thrown (expected to be caught prior in `prune_references` method)
                    }
                    else if (value.IsMap() || value.IsSequence())
                        nested_properties.push_back(value);
                }
            }
        }
    }
}

std::string structure_t::build(bool const numeric_references)
{
    validate_and_prune_hierarchy();
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
        output_json["main"] = widget_name_t("main");
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
