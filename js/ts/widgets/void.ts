import { widget_t } from "./widget";

/**
 * A purposely blank widget to fill space
 */
export class void_t extends widget_t {
    /**
     * @inheritdoc
     */
    constructor() {
        super("div", "void");
    }
    /**
     * @inheritdoc
    */
   public configuration(_configuration: object): void {
        this.content.innerText = "";
        const shadowRoot: ShadowRoot = this.content.attachShadow({
            "mode": "closed"
        });
        const noDisplayStyling: CSSStyleSheet = new CSSStyleSheet();
        noDisplayStyling.replaceSync(":host{all:initial!important;display:block!important;visibility:hidden!important;}");
        shadowRoot.adoptedStyleSheets = [noDisplayStyling];
        return;
    }
    /**
     * @inheritdoc
     */
    public async prepare(): Promise<void> {
        return Promise.resolve();
    }
};
