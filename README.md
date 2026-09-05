# SSS GUIS
An extensible YAML to graphical web application system.

---

This project is intended to generate a graphical web-based application, which can be extended with additional widgets and capabilities via a modular system of dynamically loaded modules. This project does **not** feature a server, it merely generates the files to be later used by an external server.

This project can be compiled into a standalone executable, or into a library (shared/static).

All filepaths used in this system can be relative and/or absolute. The following contents shows all filepaths as relative filepaths.

## Prerequisites
This project requires certain packages to be installed onto the system during the build process. But most are not required during runtime, for example `nodejs` is used for building the GUI environment, but is not used after the compilation of the project. To see the full list of packages required for building the executable/library, please see the contents of the file [DEPENDENCIES](DEPENDENCIES).

## Configurations
An initial configuration file must be defined, and will hold descriptions of the GUIs to generate.

The initial configuration file expects a YAML sequence (list) to be accessible under `guis`, where each GUI will be defined with the following properties:
|Property|Type|Meaning|
|-|-|-|
|`name`|`string`|The name to be be given to the GUI.|
|`config`|`string`|The location of the configuration file that holds the widget configurations.|
|`stylesheet`|`string`|The location of the stylesheet to be applied to the GUI.|
|`modules`|Sequence (list) of `string`s|*Optional* - A list of modules that are loaded into the GUI. Wildcards are allowed.|
|`dependencies`|Sequence (list) of `string`s|*Optional* - Static unmanaged dependencies. Wildcards are allowed.|
|`debug`|`boolean`|*Optional* - Wether to leave the names of widgets in the output files, otherwise it represents each widget as a numeric value.|
|`defaults`|Map of complex widget structures|*Optional* - Default values for widgets.|

Extra configurable properties for the entire project are optionally defined at the root of the YAML document (not part of the `guis` sequence):
|Property|Type|Meaning|
|-|-|-|
|`project`|`string`|*Optional* - The name of the project, this will be appended to each GUI title.|
|`project_delimiter`|`string`|*Optional* - The delimiter to be between the GUI title and the `project`.|


An example structure could look like the following:
```yaml
guis:
 - name: "Example"
   config: "example.yaml"
   stylesheet: "stylesheets.css"
   modules:
    - "example_module.js"
   dependencies:
    - "example_dependency"
   debug: false
   defaults:
     example_widget_type:
       example: property
       property: example
```

The `config` property will be used to load the main widget configuration file associated with the specific GUI.

The `defaults` map allows you to define default [Widget Configurations](#widget-configurations) for specific widget types; reducing the need to repeat common values. The top levels keys within the `defaults` map represent the `type` of the widget. If a specific widget defines a property already listed in `defaults`, its local value will **override** the default.

### Widget configuration files
Widget configuration files consist of descriptive structures used to define widgets, as well as *optional* links to additional `dependencies` defined in that file - as a sequence (list) of `string`s consisting of locations for additional configuration files - which will all be parsed resulting in additional widgets available to reference. This means that the `dependencies` keyword is reserved and widgets **cannot** be named it.

The first widget that will be used in the GUI, as the parent of all other widgets, must be named `main`; otherwise the GUI will fail to generate.

#### Widget configurations
Widget configurations are simple - by design.

Name each widget using a unique top-level key. Widget names are case-insensitive, and are internally represented as lowercase strings. Widget names must **not**: contain whitespace, or be named "null".

The core principle is that a widget needs to declare it's `type` in order for it to be valid. Additional complex YAML properties are allowed, but ultimately may not be used by the widget (defined by `type`). The property `type` will not be accessible by the widget within the generated GUI when parsing all property configurations associated to the widget, as this information would be redundant by the time the widget is being constructed. The `type` of a widget must only consist of alphanumeric characters, as well as: periods (`.`), underscores (`_`), or hyphens (`-`).

If a widget requires a child widget (defined elsewhere), then it must mention its name via an `object` reference property. Although all widgets define their own `type`, an optional strict check can be enforced by parent widgets to ensure that an `object` reference is of an expected `type`, this is done by specifying the `type` prepended with an exclamation mark (`!`) prior to the name - for example: `object: !name_of_type example_widget` - and if the referenced `object` is not of the `type` defined, the check will fail.

If a property requires a file or directory (stored within the filesystem) to be accessible to the GUI, then it should directly tag its value as a `file`. This will add the referenced filepath as a dependency - with an updated relative reference to its new location - within the generated output directory, often with different uniquely generated filenames. Tagging a value as a `file` is done by specifying the keyword prepended with an exclamation mark (`!`) prior to the path - for example: `source: !file example_file` - and if the referenced filepath cannot be resolved, the check will fail. Wildcards are **not** allowed.

##### Example widget configuration
```yaml
example_widget: # Name of widget
  type: name_of_type
  complex_properties:
    example: true
    source: !file example_file
  items:
    - object: !name_of_type another_widget
    - object: yet_another_widget
```

### Generation
Depending on the amount of `dependencies` referenced in each configuration file, generation of files may take some time... But if they are multiple `guis` defined, they will be generated in parallel.

There are 2 core generated output files of different formats, per GUI: JSON (consisting of all of the used widget definitions), and HTML (references to JSON structure, along with: `name`, `modules`, and `stylesheet`). The output file (JSON) from the `config` will be randomly named to ensure any rebuilds of the GUI points towards the latest structure configuration; however the defined `name` will still evaluate to an output file (HTML) that is not randomized, this ensures that changes can be made to the widget configuration without affecting the output file (HTML) used to access it.

The `name` to be given to the GUI serves two purposes. The first naturally being the name/title given to the GUI; but the second loosely being the filepath within the generated output directory. When populating the directory, it will attempt to remove anything that relates to: root, parent, or current; directory paths. As an example a `name` of "/example/../name" will resolve to "example/name.html" within the generated output directory.

## Generation configurations
Depending on whether you use the executable or a library version of this project, there are 5 decisions that need to be made when generating the output directory.
|Property/argument|Type|Meaning|
|-|-|-|
|Configuration file|`string`|The location of the GUI `configuration` file.|
|Output directory|`string`|The location of the output directory for all generated file.|
|Disallow conflicts|`boolean`|Whether to not allow dependencies or generated file to have conflicting output file names.|
|Flatten dependency references|`boolean`|Whether to flatten dependency output files to just their filename (no directory hierarchy).|
|Debug|`boolean` or `std::ofstream`|If using an executable, then `boolean` will be used to tell the executable to provide consistent debug to the console regarding what it is doing. If using as a library `std::ofstream` will be the stream to write debug outputs (set to `nullptr` if no debug is required).|

Use the `--help` or `-h` argument on the executable to see the specific arguments to use.

## Core widgets
### `grid`
The `grid` widget can be used to arrange subsequent widgets in a singular direction.
|Property|Type|Meaning|
|-|-|-|
|`layout`|`string`|The direction of the grid - either: "vertical", or "horizontal".|
|`ratios`|Sequence (list) of `number`s or `string`|The size ratio for each item. Use "auto" to give a widget the minimum space it needs. Use a positive number to give it a proportional fraction of the space. Each ratio corresponds to the item of at the same index.|
|`items`|Sequence (list) of `object` references|The list of widgets to put in the grid. The number of items must be equal to the number of ratios.|
|`gap`|`boolean`|*Optional* - If there should be no gap between the items (overrides stylesheet).|

#### Example `grid` YAML
```yaml
example_grid:
  type: grid
  layout: horizontal
  ratios: [2, 1, 2]
  items:
   - object: reference_to_first_widget
   - object: reference_to_second_widget
   - object: reference_to_third_widget
```

## Additional widget modules
This GUI system is modular - by design - and can be easily extended by external projects, provided that new externally provided widgets inherit from the `widget` TypeScript class, and are made known to the widget rendering subsystem. All exported TypeScript declarations are generated at compile time. Any additional widget module must be built into a bundle.

## Stylesheet
A stylesheet must be provided for each GUI!

## Executable
Additional arguments are allowed.
```console
sss-guis example/configuration.yaml generated_directory
```

## Library (C++)
The parameters of the `generate` method can be modified.
```cpp
sss::guis::guis_t("example/configuration.yaml", "generated_directory").generate(false, false, nullptr);
```
