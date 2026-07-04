import { structureWidget, widgetIdentifier_t } from "../structure";
import { widget_t } from "./widget";

/**
 * A container widget
 */
export class container_t extends widget_t {
    /**
     * @internal
    */
    protected object!: widget_t;
    /**
     * @internal
    */
    protected title!: string;
    /**
     * @inheritdoc
     */
    constructor() {
        super("fieldset", "container");
    }
    /**
     * @inheritdoc
     */
    public configuration(configuration: object): void {
        if (this.configurationHas(configuration, "title")) {
            this.title = (this.configurationGet(configuration, "title") as string);
            if (!["string", "number"].includes(typeof this.title)) {
                throw this.configurationError("A container needs a `title` to be either a string or a number");
            }
        } else {
            throw this.configurationError("A container needs a `title`");
        }
        if (this.configurationHas(configuration, "object")) {
            this.object = structureWidget(this.configurationGet(configuration, "object") as widgetIdentifier_t);
        } else {
            throw this.configurationError("A container has no reference to an `object`");
        }
    }
    /**
     * @inheritdoc
     */
    public async prepare(): Promise<void> {
        const legend: HTMLLegendElement = document.createElement("legend");
        legend.textContent = this.title;
        this.content.appendChild(legend);
        const object: HTMLElement = await this.object.render();
        this.content.appendChild(object);
    }
};
