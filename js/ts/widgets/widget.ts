/**
 * The type of a widget reference identifier
 */
export type widgetIdentifier_t = (string | number);
/**
 * Configurable properties set on a widget
 * @internal
 */
interface widgetConfigurationPropertiesSet_t {
    /**
     * The type has been set
     */
    type: boolean;
    /**
     * The name has been set
     */
    name: boolean;
};
/**
 * @abstract Base widget class
 * @template baseType The underlying HTMLElement type for the widget
 */
export abstract class widget_t<baseType extends HTMLElement = HTMLElement> {
    /**
     * Main content of the widget
     */
    protected readonly content: baseType;
    /**
     * Whether the properties of the widget have been configured
     */
    private configurationPropertiesSet: widgetConfigurationPropertiesSet_t = {
        type: false,
        name: false,
    };
    /**
     * Construct a base widget
     * @param baseTypeTag The tag of the base type to construct the widget from
     */
    constructor(baseTypeTag: string) {
        try {
            this.content = (document.createElement(baseTypeTag) as baseType);
        } catch {
            throw new Error(`Unknown widget base type: ${baseTypeTag}`);
        }
    }
    /**
     * @abstract Configure a widget
     * @param configuration Contents of widget
     */
    public abstract configuration(configuration: object): void;
    /**
     * @abstract Prepares a widget for rendering
     * @returns Promise to prepare a widget
     * @async
     */
    protected abstract prepare(): Promise<void>;
    /**
     * Configure a widget's type
     * @param type Type of widget
     * @internal
     */
    public configurationType(type: string): void {
        if (typeof type !== "string") {
            throw new Error("Widget type is invalid");
        }
        const trimmedType: string = type.trim();
        if (trimmedType.length === 0) {
            throw new Error("Widget type is not defined");
        }
        if (this.configurationPropertiesSet.type) {
            throw new Error("Widget type has already been configured");
        }
        this.content.classList.add(trimmedType);
        this.configurationPropertiesSet.type = true;
    }
    /**
     * Configure a widget's name
     * @param name Name of widget
     * @internal
     */
    public configurationName(name: widgetIdentifier_t): void {
        const originalName: (string | null) = this.content.getAttribute("name");
        if (!["string", "number"].includes(typeof name)) {
            throw new Error("Widget name is invalid");
        }
        if (originalName !== null) {
            console.warn(`The widget named "${originalName}" is being renamed to "${String(name)}"`);
        }
        if (this.configurationPropertiesSet.name) {
            throw new Error("Widget name has already been configured");
        }
        this.content.setAttribute("name", name.toString());
        this.configurationPropertiesSet.name = true;
    }
    /**
     * Renders a widget
     * @returns Widget contents
     * @async
     */
    public async render(): Promise<HTMLElement> {
        if (!this.configurationPropertiesSet.name) {
            throw new Error("Widget name has not been configured");
        }
        if (!this.configurationPropertiesSet.type) {
            throw new Error("Widget type has not been configured");
        }
        await this.prepare();
        return this.content;
    }
    /**
     * Whether a configuration has an property
     * @param configuration Configuration to check against
     * @param property The entity to search for
     * @returns Whether the configuration has an entity
     */
    protected configurationHas(configuration: object, property: string): boolean {
        try {
            const value: unknown = (configuration as Record<string, unknown>)[property];
            return (value !== undefined);
        } catch {
            return false;
        }
    }
    /**
     * Get a configuration property
     * @param configuration Configuration to abstract property from
     * @param property The property to abstract
     * @returns Configuration property
     * @throws Error when property does not exist
     */
    protected configurationGet(configuration: object, property: string): unknown {
        try {
            const value: unknown = (configuration as Record<string, unknown>)[property];
            if (value === undefined) {
                throw new Error();
            }
            return value;
        } catch {
            throw this.configurationError(`Configuration does not have an entity named "${property}"`);
        }
    }
    /**
     * Generate an error originating from a specific widget's configuration
     * @param error The error to raise (can be an Error itself)
     * @returns The generated error
     */
    protected configurationError(error: unknown): Error {
        const name: (string | null) = this.content.getAttribute("name");
        const errorType: ErrorConstructor = (error instanceof Error ? (error.constructor as ErrorConstructor) : Error);
        const formattedError: Error = new errorType(`${(name !== null && name.trim().length > 0) ? `${name}: ` : ""}${error instanceof Error ? error.message : String(error)}`);
        // If it was an Error object preserve the stack trace
        if (error instanceof Error) {
            formattedError.stack = error.stack;
        }
        return formattedError;
    }
}
