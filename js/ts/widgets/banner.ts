import { textual_t } from "./textual";

/**
 * A banner widget used to show text
 */
export class banner_t extends textual_t {
    /**
     * @inheritdoc
     */
    constructor() {
        super("h2", "banner");
    }
    /**
     * @inheritdoc
     */
    public configuration(configuration: object): void {
        super.configuration(configuration, "center", "middle");
    }
};
