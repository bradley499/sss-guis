import { widget_t } from "./widgets/widget";

/**
 * Tuple structure of widget contents
 * @first Type of widget
 * @second Configuration of widget
 * @internal
 */
type structure_widget_schema_t = [number, any];

/**
 * Interface for the structure of GUI
 * @internal
 */
interface structure_schema_t {
    /**
     * Array or object consisting of widgets
     * @variation Array Is expected by default
     * @variation Object Is expected for debug variants
     * @internal
     */
    widgets: structure_widget_schema_t[] | { [key: string]: structure_widget_schema_t };
    /**
     * Types of widgets that are used by the structure
     * @internal
     */
    types: string[];
    /**
     * Reference to the main object
     * @variation number Is expected by default
     * @variation string Is expected for debug variants
     * @internal
     */
    main: (number | string);
}

/**
 * Tuple positional references
 * @internal
 */
enum widgetData_t {
    /**
     * Reference for the type of the widget
     * @internal
     */
    widgetDataType = 0,
    /**
     * Reference for the configuration of the widget
     * @internal
     */
    widgetDataConfiguration = 1,
}

/**
 * @brief The type of a widget reference identifier
 */
export type widgetIdentifier_t = (string | number);

/**
 * Manage structural widgets
 * @internal
 */
export abstract class structure_t {
    /**
     * Declarations of widget types and factory
     * @internal
     */
    private static widgetDeclarations: { [id: string]: () => widget_t } = {};
    /**
     * Whether the structure has been successfully obtained
     * @internal
     */
    private static gotStructure: boolean = false;
    /**
     * Structure of GUI
     * @internal
     */
    private static structure: structure_schema_t;
    /**
     * Declare a widget type that applies to a factory that can be created from a structure
     * @param {string} type The widget type to construct
     * @param {function(): widget_t} widget Widget factory function to create a widget of `type`
     * @internal
     */
    public static declareWidget(type: string, widget: () => widget_t): void {
        const typeAllowedCharacters: RegExp = /^[a-z0-9._\-]+$/;
        if (!type) {
            throw new Error(`A widget must have a type defined`);
        }
        type = type.toLowerCase();
        if (!typeAllowedCharacters.test(type)) {
            throw new Error(`A widget can not be named "${type}" as it contains invalid characters`);
        } else if (type in this.widgetDeclarations) {
            throw new Error(`A widget named "${type}" has already been declared`);
        }
        this.widgetDeclarations[type] = widget;
    }
    /**
     * Asynchronously generate a structure
     * @async
     * @param {string} structure Path for structure location
     * @returns {Promise<widget_t>}
     * @internal
     */
    public static async generate(structure: string): Promise<widget_t> {
        if (!this.gotStructure) {
            if (structure == null) {
                throw new Error("No structure path was provided");
            }
            await this.get(structure).then(response => {
                this.structure = response;
                this.gotStructure = true;
            }).catch((_reason: any) => {
                throw new Error("Failed to get structure of GUI");
            });
        }
        return this.widget(this.structure.main);
    }
    /**
     * Get a widget
     * @param {widgetIdentifier_t} identifier Reference to a widget
     * @returns {widget_t} Widget
     * @internal
     */
    public static widget(identifier: widgetIdentifier_t): widget_t {
        if (!this.widgetExists(identifier)) {
            throw new Error(`No widget exists with the identifier "${identifier}"`);
        }
        // @ts-ignore - to allow either a number or a string to be an index
        let type: string = this.structure.types[this.structure.widgets[identifier][widgetData_t.widgetDataType]];
        if (!(type in this.widgetDeclarations)) {
            throw new Error(`Unable to create widget of "${type}" which is an unknown widget type`);
        }
        let widget: widget_t = this.widgetDeclarations[type]();
        // @ts-ignore - to allow either a number or a string to be an index
        widget.configuration(this.structure.widgets[identifier][widgetData_t.widgetDataConfiguration] || {});
        return widget;
    }
    /**
     * Check whether a widget exists
     * @param {widgetIdentifier_t} identifier Reference to a widget
     * @returns {boolean} Whether a widget exist
     * @internal
     */
    public static widgetExists(identifier: widgetIdentifier_t): boolean {
        if (Array.isArray(this.structure.widgets)) {
            if (typeof identifier != "number") {
                throw new Error(`Unable to check if the widget "${identifier}" exists as a numeric identifier was expected`);
            }
            return (identifier >= 0 && identifier < this.structure.widgets.length);
        } else if (typeof identifier != "string") {
            throw new Error(`Unable to check if the widget "${identifier}" exists as a string identifier was expected`);
        } else {
            return this.structure.widgets.hasOwnProperty(identifier);
        }
    }
    /**
     * Get the structure of the GUI
     * @param {structure} Path for structure location
     * @returns {Promise<structure_schema_t>}
     * @internal
     */
    private static async get(structure: string): Promise<structure_schema_t> {
        return fetch(structure).then(response => response.json()).then(response => {
            return response as structure_schema_t;
        }).catch(_error => {
            throw new Error("Failed to parse a valid JSON structure");
        });
    }
}
