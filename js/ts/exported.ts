import { alert } from "./modals/alert";
import { confirm } from "./modals/confirm";
import { dialog } from "./modals/dialog";
import { loadModule } from "./resources/module";
import { loadResource, type multimediaResource_t } from "./resources/resource";
import { loadStylesheet } from "./resources/stylesheet";
import { structure_t, type widgetIdentifier_t } from "./structure";
import { widget_t } from "./widgets/widget";

export { alert, confirm, dialog, loadModule, loadResource, multimediaResource_t, loadStylesheet, widget_t, widgetIdentifier_t};

/**
 * Declare a widget type that applies to a factory that can be created from a structure
 * @param {string} type The widget type to construct
 * @param {function(): widget_t} widget Widget factory function to create a widget of `type`
 */
export function structureDeclareWidget(type: string, widget: () => widget_t): void {
    structure_t.declareWidget(type, widget);
}
/**
 * Get a widget
 * @param {widgetIdentifier_t} identifier Reference to a widget
 * @returns {widget_t} Widget
 */
export function structureWidget(identifier: widgetIdentifier_t): widget_t {
    return structure_t.widget(identifier);
}
/**
 * Check whether a widget exists
 * @param {widgetIdentifier_t} identifier Reference to a widget
 * @returns {boolean} Whether a widget exist
 */
export function structureWidgetExists(identifier: widgetIdentifier_t): boolean {
    return structure_t.widgetExists(identifier);
}
/**
 * The event type for successful structure rendering or error
 * @returns {string} Event type
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
/**
 * Exports functions to window
 * @internal
 */
export function exportToWindow(): void {
    (window as any).structureDeclareWidget = structureDeclareWidget;
    (window as any).structureWidget = structureWidget;
    (window as any).structureWidgetExists = structureWidgetExists;
    (window as any).structureLoadEventType = structureLoadEventType;
    (window as any).widget_t = widget_t;
    (window as any).loadStylesheet = loadStylesheet;
    (window as any).loadModule = loadModule;
    (window as any).loadResource = loadResource;
    (window as any).alert = alert;
    (window as any).confirm = confirm;
    (window as any).dialog = dialog;
};
