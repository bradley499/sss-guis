import { dialog } from "./modals/dialog";
import { loadModule } from "./resources/module";
import { loadResource, type multimediaResource_t } from "./resources/resource";
import { loadStylesheet } from "./resources/stylesheet";
import { structureDeclareWidget, structureWidget, structureWidgetExists, type widgetIdentifier_t } from "./structure";
import { widget_t } from "./widgets/widget";

export { dialog, loadModule, loadResource, loadStylesheet, multimediaResource_t, structureDeclareWidget, structureWidget, structureWidgetExists, widget_t, widgetIdentifier_t};

/**
 * The event type for successful structure rendering or error
 * @returns Event type
 */
export function structureLoadEventType(): string {
    return "structureLoadEvent";
};
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
declare global {
    interface Window {
        structureDeclareWidget: typeof structureDeclareWidget;
        structureWidget: typeof structureWidget;
        structureWidgetExists: typeof structureWidgetExists;
        structureLoadEventType: typeof structureLoadEventType;
        widget_t: typeof widget_t;
        loadStylesheet: typeof loadStylesheet;
        loadModule: typeof loadModule;
        loadResource: typeof loadResource;
        dialog: typeof dialog;
    }
}
/**
 * Exports functions to window
 * @internal
 */
export function exportToWindow(): void {
    window.structureDeclareWidget = structureDeclareWidget;
    window.structureWidget = structureWidget;
    window.structureWidgetExists = structureWidgetExists;
    window.structureLoadEventType = structureLoadEventType;
    window.widget_t = widget_t;
    window.loadStylesheet = loadStylesheet;
    window.loadModule = loadModule;
    window.loadResource = loadResource;
    window.dialog = dialog;
}
