import { structure_t } from "../structure";
import { void_t } from "./void";
import { widget_t } from "./widget";

/**
 * Widgets that are stored children of a layout
 * @internal
 */
type subWidget_t = widget_t;

/**
 * A layout widget
 */
export class layout_t extends widget_t {
    /**
     * @internal
     */
    protected children!: subWidget_t[];
    constructor() {
        super("div", "layout");
        this.children = [];
    };
    public configuration(configuration: Object): void {
        if (!this.configurationHas(configuration, "columns") ||
            !Array.isArray((configuration as any).columns)) {
            throw new Error("A layout needs a numeric set of `columns`");
        }
        if (!this.configurationHas(configuration, "rows") ||
            !Array.isArray((configuration as any).rows)) {
            throw new Error("A layout needs a numeric set of `rows`");
        }
        const columns: number[] = (configuration as any).columns;
        const rows: number[] = (configuration as any).rows;
        const maxItems: number = (columns.length * rows.length);
        if (this.configurationHas(configuration, "items")) {
            if (!Array.isArray((configuration as any).items)) {
                throw new Error("A layout's `items` must be a list");
            }
            (configuration as any).items.forEach((item: any) => {
                if (this.children.length == maxItems) {
                    throw new Error("Attempting to add too many items to a layout (consider increasing `columns` or `rows`)");
                }
                if (!this.configurationHas(item, "object")) {
                    throw new Error("Layout item has no reference to an object");
                }
                if (item.object === null) {
                    this.children.push(new void_t());
                } else {
                    this.children.push(structure_t.widget(item.object));
                }
            });
        } else {
            console.warn("A layout has been created, but it has no `items`");
            return;
        }
        const columnsStyle: string = columns.map(column => {
            if (typeof column !== "number") {
                throw new Error(`A layout requires a numerical column size - not "${column}"`);
            }
            if (column == 0) {
                return "auto";
            }
            if (column < 0) {
                throw new Error("A layout can only have a column with a size that is positive");
            }
            return ("minmax(0," + column.toString() + "fr)");
        }).join(" ");
        const rowsStyle: string = rows.map(row => {
            if (typeof row !== "number") {
                throw new Error(`A layout requires a numerical row size - not "${row}"`);
            }
            if (row == 0) {
                return "auto";
            }
            if (row < 0) {
                throw new Error("A layout can only have a row with a size that is positive");
            }
            return ("minmax(0," + row.toString() + "fr)");
        }).join(" ");
        const gapStyle: string = (() => {
            if (this.configurationHas(configuration, "gap")) {
                const gap: any = (configuration as any).gap;
                if (typeof gap !== "boolean") {
                    throw new Error("A layout has a `gap` property but is not a boolean value");
                }
                if (gap === false) {
                    return "gap:0!important;grid-gap:0!important;";
                }
            }
            return "";
        })();
        // Fill the remaining cells...
        for (let i = this.children.length; i < maxItems; i++) {
            this.children.push(new void_t());
        }
        const layoutShadowRoot: ShadowRoot = this.content.attachShadow({ mode: "closed" });
        const noInheritedStyling: CSSStyleSheet = new CSSStyleSheet();
        noInheritedStyling.replaceSync(`:host{display:grid!important;grid-template-columns:${columnsStyle}!important;grid-template-rows:${rowsStyle}!important;${gapStyle}}`);
        layoutShadowRoot.adoptedStyleSheets = [noInheritedStyling];
        const slot = document.createElement("slot");
        layoutShadowRoot.appendChild(slot);
    }
    public render(): Promise<HTMLElement> {
        return new Promise<HTMLElement>(async (resolve, reject) => {
            try {
                let childrenPromises: Promise<HTMLElement>[] = [];
                this.children.forEach(child => {
                    childrenPromises.push(child.render());
                });
                await Promise.all(childrenPromises).then(async children => {
                    await children.forEach(async (object) => {
                        this.content.appendChild(await object);
                    });
                });
                resolve(this.content);
            } catch (error) {
                if (error instanceof RangeError) {
                    reject("Failed to render layout (it is possible that a child item may be recursive)");
                }
                reject(error);
            }
        });
    }
};
