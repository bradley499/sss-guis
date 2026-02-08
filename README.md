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
|`dependencies`|Sequence (list) of `string`s|*Optional* - Static unmanaged dependencies that the GUI can use (such as multimedia). Wildcards are allowed.|
|`debug`|`boolean`|*Optional* - Wether to leave the names of widgets in the output files, otherwise it represents each widget as a numeric value.|

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
```

The `config` property will be used to load the main widget configuration file associated with the specific GUI.

### Widget configuration files
Widget configuration files consist of descriptive structures used to define widgets, as well as *optional* links to additional `dependencies` defined in that file - as a sequence (list) of `string`s consisting of locations for additional configuration files - which will all be parsed resulting in additional widgets available to reference. This means that the `dependencies` keyword is reserved and widgets **cannot** be named it.

The first widget that will be used in the GUI, as the parent of all other widgets, must be named `main`; otherwise the GUI will fail to generate.

#### Widget configurations
Widget configurations are simple - by design.

Name each widget using a unique top-level key. Widget names are case-insensitive, and are internally represented as lowercase strings. Widget names must **not**: contain whitespace, or be named "null".

The core principle is that a widget needs to declare it's `type` in order for it to be valid. Additional complex YAML properties are allowed, but ultimately may not be used by the widget (defined by `type`). The property `type` will not be accessible by the widget within the generated GUI when parsing all property configurations associated to the widget, as this information would be redundant by the time the widget is being constructed. The `type` of a widget must only consist of alphanumeric characters, as well as: periods (`.`), underscores (`_`), or hyphens (`-`).

If a widget requires a child widget (defined elsewhere), then it must mention its name via an `object` reference property. Although all widgets define their own `type`, an optional strict check can be enforced by parent widgets to ensure that an `object` reference is of an expected `type`, this is done by specifying the `type` prepended with an exclamation mark (`!`) prior to the name - for example: `object: !name_of_type example_widget` - and if the referenced `object` is not of the `type` defined, the check will fail.

##### Example widget configuration
```yaml
example_widget: # Name of widget
  type: name_of_type
  complex_properties:
    example: true
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
The following widgets are built into the core GUI system and can be directly used via setting a widget's `type` property to one of the following:

### Structural
#### `layout`
Used to define a grid structure that houses widgets within each cell.
|Property|Type|Meaning|
|-|-|-|
|`rows`|Sequence (list) of `number`s|The ratios that the `rows` will occupy. If the number is `0` then that lets the widget occupy the minimum amount of space that it needs.|
|`columns`|Sequence (list) of `number`s|The ratios that the `columns` will occupy. If the number is `0` then that lets the widget occupy the minimum amount of space that it needs.|
|`items`|Sequence (list) of `object` references|A collection of `object` references to other widgets. The collection must not be larger than would fit in the amount of defined `rows` and `columns`.|
|`gap`|`boolean`|*Optional* - If there should be no gap between the items (overrides stylesheet).|

#### `container`
Wraps a child widget within a named container.
|Property|Type|Meaning|
|-|-|-|
|`title`|`string` or `number`|The title that should be given to the container.|
|`object`|`object`|The child object of the container.|

#### `tabs`
Have a tabular collection of widgets with corresponding names for each view.
|Property|Type|Meaning|
|-|-|-|
|`items`|Sequence (list) of [`Tab items`](#tab-items)|See [Tab items](#tab-items).|
|`position`|`string`|*Optional* - The location of the tab buttons: "top", "bottom", "left", or "right".|
##### Tab items
|Property|Type|Meaning|
|-|-|-|
|`name`|`string` or `number`|The name of the tab|
|`object`|`object`|The object to be shown on the tab.|

#### `null`
Does not have anything that is shown to a user of the GUI. This widget can be used to fill space.

### Textual
#### `banner`
Used to show large text.
|Property|Type|Meaning|
|-|-|-|
|`text`|`string` or `number` or `boolean`|The text to be shown.|
|`alignment`|[`Textual alignment`](#textual-alignment)|*Optional* - See [Textual alignment](#textual-alignment)|
|`color`|`string`|*Optional* - The color of the text. Must be a valid CSS color.|

#### `text`
Used to show a lot of text.
|Property|Type|Meaning|
|-|-|-|
|`text`|`string` or `number` or `boolean`|The text to be shown.|
|`alignment`|[`Textual alignment`](#textual-alignment)|*Optional* - See [Textual alignment](#textual-alignment)|
|`color`|`string`|*Optional* - The color of the text. Must be a valid CSS color.|

#### Textual alignment
|Property|Type|Meaning|
|-|-|-|
|`horizontal`|`string`|*Optional* - The horizontal alignment of the text: "left", "center", or "right".|
|`vertical`|`string`|*Optional* - The vertical alignment of the text: "top", "middle", or "bottom".|

### Multimedia
#### `audio`
Used to load an audio player into the GUI. This widget checks whether the GUI environment supports the audio format.
|Property|Type|Meaning|
|-|-|-|
|`source`|`string`|The location of the audio source file.|

#### `image`
Used to load an image into the GUI. This widget checks whether the GUI environment supports the image format.
|Property|Type|Meaning|
|-|-|-|
|`source`|`string`|The location of the image source file.|
|`contain`|`string`|*Optional* - This can be set to either to: "fit" the image fit its entire content within the widget, or "fill" the entire widget with the content (some content may be lost).|

#### `video`
Used to load an video player into the GUI. This widget checks whether the GUI environment supports the video format.
|Property|Type|Meaning|
|-|-|-|
|`source`|`string`|The location of the video source file.|
|`contain`|`string`|*Optional* - This can be set to either to: "fit" the video fit its entire content within the widget, or "fill" the entire widget with the content (some content may be lost).|

### Additional widget modules
This GUI system is modular - by design - and can be easily extended by external projects, provided that new externally provided widgets inherit from the `widget_t` TypeScript class, and are made known to the widget rendering subsystem. All exported TypeScript declarations are generated at compile time, and are populated into the build artifact `guis.d.ts`.

## Stylesheet
This project is part of a broader collection of repositories. To make the best use of this project, please use a stylesheet. Stylesheets are designed to be configurable and extensible; but as a minimum requirement the core components of the following stylesheet must be used: [sss-guis-stylesheet-core](https://github.com/bradley499/sss-guis-stylesheet-core).

An example of a complete stylesheet built to make full use of the [Core Widgets](#core-widgets), can be found at [sss-guis-stylesheet-default](https://github.com/bradley499/sss-guis-stylesheet-default).

## Example
There is a directory named [`example`](example) within this project. This is a very simple example to showcase how to construct a collection of GUIs containing some example widgets. To generate a GUI based on the contents within the [`example`](example) directory, please follow one of the following approaches:

### Executable
Additional arguments are allowed.
```console
sss-guis example/configuration.yaml generated_directory
```

### Library (C++)
The parameters of the `generate` method can be modified.
```cpp
sss::guis::guis_t("example/configuration.yaml", "generated_directory").generate(false, false, nullptr);
```
