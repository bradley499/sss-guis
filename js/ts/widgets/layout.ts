import { structureDeclareWidget, structureWidget } from "../structure";
import { void_t } from "./void";
import { widget_t, widgetIdentifier_t } from "./widget";

/**
 * Widgets that are stored children of a layout
 * @internal
 */
type subWidget_t = widget_t;

/**
 * A layout widget
 */
export class layout_t extends widget_t<HTMLDivElement> {
    /**
     * @internal
    */
    protected children!: subWidget_t[];
    /**
     * @inheritdoc
     */
    constructor() {
        super("div");
    }
    /**
     * @inheritdoc
     */
    public configuration(configuration: object): void {
        if (!this.configurationHas(configuration, "columns") ||
            !Array.isArray(this.configurationGet(configuration, "columns"))) {
            throw this.configurationError("A layout needs a numeric set of `columns`");
        }
        if (!this.configurationHas(configuration, "rows") ||
            !Array.isArray(this.configurationGet(configuration, "rows"))) {
            throw this.configurationError("A layout needs a numeric set of `rows`");
        }
        const columns: number[] = (this.configurationGet(configuration, "columns") as number[]);
        const rows: number[] = (this.configurationGet(configuration, "rows") as number[]);
        const maxItems: number = (columns.length * rows.length);
        this.children = [];
        if (this.configurationHas(configuration, "items")) {
            const items: object[] = (this.configurationGet(configuration, "items") as object[]);
            if (!Array.isArray(items)) {
                throw this.configurationError("A layout's `items` must be a list");
            }
            items.forEach((item: object) => {
                if (this.children.length == maxItems) {
                    throw this.configurationError("Attempting to add too many items to a layout (consider increasing `columns` or `rows`)");
                }
                if (!this.configurationHas(item, "object")) {
                    throw this.configurationError("Layout item has no reference to an `object`");
                }
                type nullableWidgetIdentifier_t = (widgetIdentifier_t | null);
                const objectIdentifier: nullableWidgetIdentifier_t = (this.configurationGet(item, "object") as nullableWidgetIdentifier_t);
                if (objectIdentifier === null) {
                    this.children.push(new void_t());
                } else {
                    this.children.push(structureWidget(objectIdentifier));
                }
            });
        } else {
            console.warn("A layout has been created, but it has no `items`");
            return;
        }
        const columnsStyle: string = columns.map((column: number) => {
            if (typeof column !== "number") {
                throw this.configurationError(`A layout requires a numerical column size - not "${String(column)}"`);
            }
            if (column == 0) {
                return "auto";
            }
            if (column < 0) {
                throw this.configurationError("A layout can only have a column with a size that is positive");
            }
            return ("minmax(0," + column.toString() + "fr)");
        }).join(" ");
        const rowsStyle: string = rows.map((row: number) => {
            if (typeof row !== "number") {
                throw this.configurationError(`A layout requires a numerical row size - not "${String(row)}"`);
            }
            if (row == 0) {
                return "auto";
            }
            if (row < 0) {
                throw this.configurationError("A layout can only have a row with a size that is positive");
            }
            return ("minmax(0," + row.toString() + "fr)");
        }).join(" ");
        let gapStyle: string = "";
        if (this.configurationHas(configuration, "gap")) {
            const gap: unknown = this.configurationGet(configuration, "gap");
            if (typeof gap !== "boolean") {
                throw this.configurationError("A layout has a `gap` property but is not a boolean value");
            }
            if (!gap) {
                gapStyle = "gap:0!important;grid-gap:0!important;";
            }
        }
        // Fill the remaining cells...
        for (let i: number = this.children.length; i < maxItems; i++) {
            this.children.push(new void_t());
        }
        const layoutShadowRoot: ShadowRoot = this.content.attachShadow({
            mode: "closed"
        });
        const noInheritedStyling: CSSStyleSheet = new CSSStyleSheet();
        noInheritedStyling.replaceSync(`:host{display:grid!important;grid-template-columns:${columnsStyle}!important;grid-template-rows:${rowsStyle}!important;${gapStyle}}`);
        layoutShadowRoot.adoptedStyleSheets = [noInheritedStyling];
        const slot: HTMLSlotElement = document.createElement("slot");
        layoutShadowRoot.appendChild(slot);
    }
    /**
     * @inheritdoc
     */
    public async prepare(): Promise<void> {
        const childrenPromises: Promise<HTMLElement>[] = [];
        this.children.forEach((children: subWidget_t) => {
            childrenPromises.push(children.render());
        });
        const children: HTMLElement[] = await Promise.all(childrenPromises);
        for (const object of children) {
            this.content.appendChild(object);
        }
    }
};

structureDeclareWidget("layout", layout_t);
