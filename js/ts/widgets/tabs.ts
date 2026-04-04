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
            this.raiseError("A tabs widget is missing `items`");
        }
        ((configuration as any).items as Object[]).forEach((item: any) => {
            if (!this.configurationHas(item, "name")) {
                this.raiseError("A tab `name` is expected");
            }
            if (!(typeof item.name === "string" || typeof item.name === "number")) {
                this.raiseError("A tab `name` needs to be either a string or a number");
            }
            const tabName: string = item.name.toString();
            if (tabName in this.tabs) {
                this.raiseError(`Another tab exists with the name of "${tabName}" within a tab widget`);
            }
            if (!this.configurationHas(item, "object")) {
                this.raiseError(`Tab "${tabName}" has no reference to an \`object\``);
            }
            this.tabs[tabName] = structure_t.widget(item.object);
        });
        if (Object.keys(this.tabs).length == 0) {
            this.raiseError("A tabs widget is missing `items`");
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
                            this.raiseError(`"${position}" is not a valid position for a collection of tabs`);
                    }
                    break;
                }
                this.raiseError("The `position` for a collection of tabs must be a string");
            } while (false);
        }
        this.content.setAttribute("position", this.position);
    }
    public render(): Promise<HTMLElement> {
        return new Promise<HTMLElement>(async (resolve, reject) => {
            const tabButtonContainer: HTMLDivElement = document.createElement("div");
            const tabView: HTMLDivElement = document.createElement("div");
            const tabButtons: HTMLButtonElement[] = [];
            let firstTab: boolean = true;
            try {
                let tabsPromises: { [key: string]: Promise<HTMLElement> } = {};
                Object.entries(this.tabs).forEach(
                    ([tab, widget]) => {
                        tabsPromises[tab] = widget.render();
                    });
                await Promise.all(Object.values(tabsPromises));
                await Object.entries(tabsPromises).forEach(async ([tab, widget]) => {
                    const tabObject: HTMLElement = await widget;
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
                });
                this.content.appendChild(tabButtonContainer);
                this.content.appendChild(tabView);
                resolve(this.content);
            } catch (error) {
                reject(error);
            }
        });
    }
};
