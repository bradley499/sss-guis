import { structureDeclareWidget } from "../structure";
import { textual_t } from "./textual";

/**
 * A textual widget used to show text
 */
export class text_t extends textual_t<HTMLParagraphElement> {
    /**
     * @inheritdoc
     */
    constructor() {
        super("p");
    }
    /**
     * @inheritdoc
     */
    public configuration(configuration: object): void {
        super.configuration(configuration, "left", "middle");
    }
};

structureDeclareWidget("text", text_t);
