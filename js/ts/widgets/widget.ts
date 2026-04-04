/**
 * @abstract Base widget class
 */
export abstract class widget_t {
    /**
     * Main HTMLElement of the widget
     */
    protected content!: HTMLElement;
    /**
     * Construct a base widget
     * @param {string} baseType The base type to construct the widget from
     * @param {string} type The type of the widget
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
     * Configure a widget's name
     * @param {string} name Name of widget
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
     * @abstract Configure a widget
     * @param {Object} configuration Contents of widget
     */
    public abstract configuration(configuration: Object): void;
    /**
     * @abstract Render a widget
     * @returns {HTMLElement}
     */
    public abstract render(): Promise<HTMLElement>;
    /**
     * Whether a configuration has an entity
     * @param {Object} configuration Configuration to check against
     * @param {string} entity The entity to search for
     * @returns {boolean} Whether the configuration has an entity
     */
    protected configurationHas(configuration: Object, entity: string): boolean {
        try {
            const value: any = (configuration as any)[entity];
            return (value !== undefined);
        } catch {
            return false;
        }
    }
    /**
     * Raise an error originating from a specific widget
     * @param {any} error The error to raise (can be an Error itself)
     */
    protected raiseError(error: any): never {
        const name: (string | null) = this.content.getAttribute("name");
        const errorType: any = (error instanceof Error ? (error.constructor as any) : Error);
        const formattedError = new errorType(`${(name !== null && name.trim().length > 0) ? `${name}: ` : ""}${error instanceof Error ? error.message : String(error)}`);
        // If it was an Error object preserve the stack trace
        if (error instanceof Error) {
            formattedError.stack = error.stack;
        }
        throw formattedError;
    }
};
