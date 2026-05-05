import { structure_t } from "../structure";
import { widget_t } from "./widget";

/**
 * Widgets that are stored children of a tabs
 * @internal
 */
type subWidget_t = widget_t;

/**
 * The position of the tab buttons
 * @internal
 */
type tabsPosition_t = ("top" | "right" | "bottom" | "left");

/**
 * A tabs widget
 */
export class tabs_t extends widget_t {
    /**
     * @internal
     */
    protected tabs!: { [key: string]: subWidget_t };
    protected position!: tabsPosition_t;
    constructor() {
        super("div", "tabs");
        this.tabs = {};
        this.position = "top";
    };
    public configuration(configuration: Object): void {
        if (!this.configurationHas(configuration, "items") ||
            !Array.isArray((configuration as any).items)) {
            throw this.configurationError("A tabs widget is missing `items`");
        }
        ((configuration as any).items as Object[]).forEach((item: any) => {
            if (!this.configurationHas(item, "name")) {
                throw this.configurationError("A tab `name` is expected");
            }
            if (!(typeof item.name === "string" || typeof item.name === "number")) {
                throw this.configurationError("A tab `name` needs to be either a string or a number");
            }
            const tabName: string = item.name.toString();
            if (tabName in this.tabs) {
                throw this.configurationError(`Another tab exists with the name of "${tabName}" within a tab widget`);
            }
            if (!this.configurationHas(item, "object")) {
                throw this.configurationError(`Tab "${tabName}" has no reference to an \`object\``);
            }
            this.tabs[tabName] = structure_t.widget(item.object);
        });
        if (Object.keys(this.tabs).length == 0) {
            throw this.configurationError("A tabs widget is missing `items`");
        }
        if (this.configurationHas(configuration, "position")) {
            do {
                if (typeof (configuration as any).position === "string") {
                    const position: tabsPosition_t = (configuration as any).position;
                    switch (position) {
                        case "top":
                        case "right":
                        case "bottom":
                        case "left":
                            this.position = position;
                            break;
                        default:
                            throw this.configurationError(`"${position}" is not a valid position for a collection of tabs`);
                    }
                    break;
                }
                throw this.configurationError("The `position` for a collection of tabs must be a string");
            } while (false);
        }
        this.content.setAttribute("position", this.position);
        const layoutShadowRoot: ShadowRoot = this.content.attachShadow({ mode: "closed" });
        const noInheritedStyling: CSSStyleSheet = new CSSStyleSheet();
        noInheritedStyling.replaceSync(":host{display:flex!important}:host([position=top]){flex-direction:column!important}:host([position=bottom]){flex-direction:column-reverse!important}:host([position=left]){flex-direction:row!important}:host([position=right]){flex-direction:row-reverse!important}::slotted(div:first-of-type){display:flex!important;overflow:auto;min-width:min-content;width:auto;height:auto}:host([position=bottom]) ::slotted(div:first-of-type),:host([position=top]) ::slotted(div:first-of-type){flex-direction:row}:host([position=left]) ::slotted(div:first-of-type),:host([position=right]) ::slotted(div:first-of-type){flex-direction:column}");
        layoutShadowRoot.adoptedStyleSheets = [noInheritedStyling];
        const slot: HTMLSlotElement = document.createElement("slot");
        layoutShadowRoot.appendChild(slot);
    }
    public render(): Promise<HTMLElement> {
        return new Promise<HTMLElement>(async (resolve, reject) => {
            const tabButtonContainer: HTMLDivElement = document.createElement("div");
            const tabView: HTMLDivElement = document.createElement("div");
            const tabButtons: HTMLButtonElement[] = [];
            let firstTab: boolean = true;
            try {
                let tabsPromises: { [key: string]: Promise<HTMLElement> } = {};
                Object.entries(this.tabs).forEach(([tab, widget]) => {
                    tabsPromises[tab] = widget.render();
                });
                await Promise.all(Object.values(tabsPromises));
                for (const [tab, widgetPromise] of Object.entries(tabsPromises)) {
                    const tabObject: HTMLElement = await widgetPromise;
                    const tabButton: HTMLButtonElement = document.createElement("button");
                    tabButton.innerText = tab;
                    tabButtons.push(tabButton);
                    tabButtonContainer.appendChild(tabButton);
                    tabButton.addEventListener("click", () => {
                        if (tabView.firstElementChild !== tabObject) {
                            tabView.replaceChildren(tabObject);
                        }
                        tabButtons.forEach((button: HTMLButtonElement) => {
                            if (button !== tabButton) {
                                button.removeAttribute("active");
                            }
                        });
                        tabButton.setAttribute("active", "");
                    });
                    if (firstTab) {
                        tabButton.click();
                        firstTab = false;
                    }
                }
                this.content.appendChild(tabButtonContainer);
                this.content.appendChild(tabView);
                resolve(this.content);
            } catch (error) {
                reject(error);
            }
        });
    }

};
