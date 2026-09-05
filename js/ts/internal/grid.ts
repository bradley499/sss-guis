import { structureDeclareWidget, structureWidget } from "../structure";
import { widget, widgetIdentifier } from "../widget";

/**
 * Widgets that are stored children of a grid
 * @internal
 */
type subWidget = widget;
/**
 * A grid widget
 */
export class grid extends widget {
    /**
     * @internal
    */
    protected children!: subWidget[];
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
        if (!this.configurationHas(configuration, "layout")) {
            throw this.configurationError("A grid needs a `layout`");
        }
        type layoutType = ("horizontal" | "vertical");
        const layout: layoutType = this.configurationGet<layoutType>(configuration, "layout");
        if (typeof layout !== "string" || !["horizontal", "vertical"].includes(layout)) {
            throw this.configurationError("A grid needs a `layout` to be either `horizontal` or `vertical`");
        }
        if (!this.configurationHas(configuration, "ratios") ||
            !Array.isArray(this.configurationGet(configuration, "ratios"))) {
            throw this.configurationError("A grid needs a set of `ratios`");
        }
        this.children = [];
        if (this.configurationHas(configuration, "items")) {
            const items: object[] = this.configurationGet<object[]>(configuration, "items");
            if (!Array.isArray(items)) {
                throw this.configurationError("A grid's `items` must be a list");
            }
            items.forEach((item: object) => {
                if (!this.configurationHas(item, "object")) {
                    throw this.configurationError("Grid item has no reference to an `object`");
                }
                this.children.push(structureWidget(this.configurationGet<widgetIdentifier>(item, "object")));
            });
        } else {
            throw this.configurationError("A grid must have `items` defined");
        }
        type ratioType = (number | "auto");
        const ratios: ratioType[] = this.configurationGet<ratioType[]>(configuration, "ratios");
        if (ratios.length != this.children.length) {
            throw this.configurationError("Grid `items` do not match the amount defined in its `ratios`");
        }
        const ratiosStyle: string = ratios.map((ratio: ratioType) => {
            const auto: boolean = (ratio === "auto");
            if (typeof ratio !== "number" && !auto) {
                throw this.configurationError(`A grid requires a numerical ratio size - not "${ratio}"`);
            }
            if (auto) {
                return "auto";
            } else if ((ratio as number) < 0) {
                throw this.configurationError("A grid can only have a ratio with a size that is positive");
            }
            return ("minmax(0," + ratio.toString() + "fr)");
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
        const layoutShadowRoot: ShadowRoot = this.content.attachShadow({
            mode: "closed"
        });
        const noInheritedStyling: CSSStyleSheet = new CSSStyleSheet();
        noInheritedStyling.replaceSync(`:host{display:grid!important;${layout === "horizontal" ? "grid-template-rows:auto!important;grid-template-columns" : "grid-template-columns:auto!important;grid-template-rows"}:${ratiosStyle}!important;${gapStyle}}slot{display:contents!important}`);
        layoutShadowRoot.adoptedStyleSheets = [noInheritedStyling];
        const slot: HTMLSlotElement = document.createElement("slot");
        layoutShadowRoot.appendChild(slot);
    }
    /**
     * @inheritdoc
     */
    public async prepare(): Promise<void> {
        const childrenPromises: Promise<HTMLElement>[] = [];
        this.children.forEach((children: subWidget) => {
            childrenPromises.push(children.render());
        });
        const children: HTMLElement[] = await Promise.all(childrenPromises);
        for (const object of children) {
            this.content.appendChild(object);
        }
    }
};

structureDeclareWidget("grid", grid);
