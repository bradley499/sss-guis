import { textual_t } from "./textual";

/**
 * A textual widget used to show text
 */
export class text_t extends textual_t {
    /**
     * @inheritdoc
     */
    constructor() {
        super("span", "text");
    }
    /**
     * @inheritdoc
     */
    public configuration(configuration: object): void {
        super.configuration(configuration, "left", "middle");
    }
};
