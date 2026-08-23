import { structureDeclareWidget } from "../structure";
import { textual_t } from "./textual";

/**
 * A banner widget used to show text
 */
export class banner_t extends textual_t<HTMLHeadingElement> {
    /**
     * @inheritdoc
     */
    constructor() {
        super("h2");
    }
    /**
     * @inheritdoc
     */
    public configuration(configuration: object): void {
        super.configuration(configuration, "center", "middle");
    }
};

structureDeclareWidget("banner", banner_t);
