import { loadResource, multimediaResource } from "./resource";
import { widget, type widgetIdentifier } from "./widget";

/**
 * Tuple structure of widget contents: type of widget, configuration of widget
 * @internal
 */
type structureWidgetSchema = [number, object];
/**
 * Interface for the structure of GUI
 * @internal
 */
interface structureSchema {
    /**
     * Array or object consisting of widgets
     * @variation Array Is expected by default
     * @variation Object Is expected for debug variants
     * @internal
     */
    widgets: (structureWidgetSchema[] | Record<string, structureWidgetSchema>);
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
enum widgetData {
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
 * Declarations of widget types and factory
 * @internal
 */
const widgetDeclarations: Record<string, () => widget> = {};
/**
 * Structure of GUI
 * @internal
 */
let structure: structureSchema;
/**
 * Get the structure of the GUI
 * @param structurePath Path for structure location
 * @returns Promise of widget structure
 * @internal
 */
async function get(structurePath: string): Promise<structureSchema> {
    return loadResource(structurePath).then((response: multimediaResource) => fetch(response.blobUrl)).then((fetchResponse: Response) => fetchResponse.json()).then((response: unknown) => {
        return (response as structureSchema);
    }).catch((_error: unknown) => {
        throw new Error("Failed to parse a valid JSON structure");
    });
}
/**
 * Declare a widget type that applies to a factory that can be created from a structure
 * @param type The widget type to construct
 * @param widget Widget factory function to create a widget of `type`
 */
export function structureDeclareWidget(type: string, widget: new () => widget): void {
    const typeAllowedCharacters: RegExp = /^[a-z0-9._-]+$/;
    if (!type) {
        throw new Error(`A widget must have a type defined`);
    }
    type = type.toLowerCase();
    if (!typeAllowedCharacters.test(type)) {
        throw new Error(`A widget can not be named "${type}" as it contains invalid characters`);
    } else if (type in widgetDeclarations) {
        throw new Error(`A widget named "${type}" has already been declared`);
    }
    /**
     * Store widget constructor
     * @returns Widget constructor
     */
    widgetDeclarations[type] = (): widget => new widget();
}
/**
 * Asynchronously generate a structure
 * @async
 * @param structurePath Path for structure location
 * @returns Promise of generated widget structure
 */
export async function structureGenerate(structurePath: string): Promise<widget> {
    await get(structurePath).then((response: structureSchema) => {
        structure = response;
    }).catch((_reason: unknown) => {
        throw new Error("Failed to get structure of GUI");
    });
    return structureWidget(structure.main);
}
/**
 * The event emitted on the successful completion of structure rendering or on error
 */
export interface structureLoadEvent {
    /**
     * Whether the structure loaded successfully
     */
    success: boolean;
    /**
     * Message to be set if an error occurred
     */
    message?: string;
};
/**
 * The event for successful structure rendering or error
 */
export const structureLoadEventType: string = "structureLoadEvent";
/**
 * Get a widget
 * @param identifier Reference to a widget
 * @returns Widget
 */
export function structureWidget(identifier: widgetIdentifier): widget {
    if (!structureWidgetExists(identifier)) {
        throw new Error(`No widget exists with the identifier "${String(identifier)}"`);
    }
    const type: string = structure.types[(structure.widgets as Record<string | number, structureWidgetSchema>)[identifier][widgetData.widgetDataType]];
    if (!(type in widgetDeclarations)) {
        throw new Error(`Unable to create widget of "${type}" which is an unknown widget type`);
    }
    const generatedWidget: widget = widgetDeclarations[type]();
    generatedWidget.configurationType(type);
    generatedWidget.configurationName(identifier.toString());
    generatedWidget.configuration((structure.widgets as Record<string | number, structureWidgetSchema>)[identifier][widgetData.widgetDataConfiguration] ?? {});
    return generatedWidget;
}
/**
 * Check whether a widget exists
 * @param identifier Reference to a widget
 * @returns Whether a widget exist
 */
export function structureWidgetExists(identifier: widgetIdentifier): boolean {
    if (Array.isArray(structure.widgets)) {
        if (typeof identifier != "number") {
            throw new Error(`Unable to check if the widget "${identifier}" exists as a numeric identifier was expected`);
        }
        return (identifier >= 0 && identifier < structure.widgets.length);
    } else if (typeof identifier != "string") {
        throw new Error(`Unable to check if the widget "${String(identifier)}" exists as a string identifier was expected`);
    } else {
        return Object.prototype.hasOwnProperty.call(structure.widgets, identifier);
    }
}
