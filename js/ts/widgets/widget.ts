/**
 * @abstract Base widget class
 */
export abstract class widget_t {
    /**
     * Main HTMLElement of the widget
     */
    protected readonly content!: HTMLElement;
    /**
     * Construct a base widget
     * @param baseType The base type to construct the widget from
     * @param type The type of the widget
     */
    constructor(baseType: string, type: string) {
        try {
            this.content = document.createElement(baseType);
        } catch {
            throw new Error(`Unknown widget base type: ${baseType}`);
        }
        if (type.trim().length == 0) {
            throw new Error("Widget type is not defined");
        }
        this.content.className = type;
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
     * Configure a widget's name
     * @param name Name of widget
     * @internal
     */
    public configurationName(name: string): void {
        const originalName: (string | null) = this.content.getAttribute("name");
        if (originalName !== null) {
            console.warn(`The widget named "${originalName}" is being renamed to "${name}"`);
        }
        this.content.setAttribute("name", name);
    }
    /**
     * Renders a widget
     * @returns Widget contents
     * @async
     */
    public async render(): Promise<HTMLElement> {
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
