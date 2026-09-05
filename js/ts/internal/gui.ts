/**
 * Interface for the GUI configuration
 * @internal
 */
export interface guiSchema {
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
declare const gui: (guiSchema | undefined);

/**
 * The accessible GUI configuration
 * @returns The current configuration for this GUI
 * @internal
 */
export function getGui(): guiSchema {
    if (typeof gui === "undefined") {
        throw new Error("No GUI configuration was declared");
    }
    const guiData: guiSchema = {
        modules: gui.modules,
        name: gui.name,
        project: gui.project,
        structure: gui.structure,
        stylesheet: gui.stylesheet,
    };
    return guiData;
}
