/**
 * Interface for the GUI configuration
 * @internal
 */
interface gui_schema_t {
    /**
     * The modules to load during startup of GUI
     * @internal
     */
    modules: string[];
    /**
     * The name/title of the GUI
     * @internal
     */
    name: string;
    /**
     * The name of the project
     * @internal
     */
    project: string;
    /**
     * The path to the structure of the GUI
     * @internal
     */
    structure: string;
    /**
     * The stylesheet to apply to the GUI
     * @internal
     */
    stylesheet: string;
}

/**
 * The provided GUI configuration - expected to be available from host document
 * @internal
 */
declare const gui: (gui_schema_t | undefined);

/**
 * The accessible GUI configuration
 * @internal
 */
export class gui_t {
    /**
     * The modules to load during startup of GUI
     * @internal
     */
    public modules!: string[];
    /**
     * The name/title of the GUI
     * @internal
     */
    public name!: string;
    /**
     * The name of the project
     * @internal
     */
    public project!: string;
    /**
     * The path to the structure of the GUI
     * @internal
     */
    public structure!: string;
    /**
     * The stylesheet to apply to the GUI
     * @internal
     */
    public stylesheet!: string;
    /**
     * Validates a GUI configuration
     */
    constructor() {
        if (typeof gui === "undefined") {
            throw new Error("No GUI configuration was declared");
        }
        this.modules = gui.modules;
        this.name = gui.name;
        this.project = gui.project;
        this.structure = gui.structure;
        this.stylesheet = gui.stylesheet;
    }
}
