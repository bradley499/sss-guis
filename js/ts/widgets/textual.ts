import { widget_t } from "./widget";

/**
 * The horizontal alignment of a textual widget
 * @internal
 */
type textualAlignmentHorizontal_t = ("left" | "right" | "center");
/**
 * The vertical alignment of a textual widget
 * @internal
 */
type textualAlignmentVertical_t = ("top" | "middle" | "bottom");
/**
 * The horizontal and vertical alignment configuration
 * @internal
 */
interface textualAlignmentConfiguration {
    horizontal: textualAlignmentHorizontal_t;
    vertical: textualAlignmentVertical_t;
};
/**
 * The HTML attribute for horizontal alignment of a textual widget
 * @internal
 */
const textualAlignmentHorizontalAttribute: string = "align";
/**
 * The HTML attribute for vertical alignment of a textual widget
 * @internal
 */
const textualAlignmentVerticalAttribute: string = "valign";

/**
 * A textual widget used to show text
 */
export abstract class textual_t extends widget_t {
    /**
     * @param textualAlignmentHorizontalDefault default horizontal alignment
     * @param textualAlignmentVerticalDefault default vertical alignment
     * @inheritdoc
     */
    public override configuration(configuration: object, textualAlignmentHorizontalDefault: textualAlignmentHorizontal_t = "left", textualAlignmentVerticalDefault: textualAlignmentVertical_t = "middle"): void {
        if (!this.configurationHas(configuration, "text")) {
            throw this.configurationError(`A ${this.content.className} widget requires \`text\` to be shown`);
        }
        this.content.innerText = String(this.configurationGet(configuration, "text"));
        if (this.configurationHas(configuration, "alignment")) {
            const alignment: textualAlignmentConfiguration = (this.configurationGet(configuration, "alignment") as textualAlignmentConfiguration);
            if (this.configurationHas(alignment, "horizontal")) {
                const horizontalAlignment: textualAlignmentHorizontal_t = alignment.horizontal;
                switch (horizontalAlignment) {
                    case "left":
                    case "right":
                    case "center":
                        this.content.setAttribute(textualAlignmentHorizontalAttribute, horizontalAlignment);
                        break;
                    default:
                        throw this.configurationError(`"${String(horizontalAlignment)}" is not a valid \`horizontal\` \`alignment\` for a ${this.content.className} widget`);
                }
            } else {
                this.content.setAttribute(textualAlignmentHorizontalAttribute, textualAlignmentHorizontalDefault);
            }
            if (this.configurationHas(alignment, "vertical")) {
                const verticalAlignment: textualAlignmentVertical_t = alignment.vertical;
                switch (verticalAlignment) {
                    case "top":
                    case "bottom":
                    case "middle":
                        this.content.setAttribute(textualAlignmentVerticalAttribute, verticalAlignment);
                        break;
                    default:
                        throw this.configurationError(`"${String(verticalAlignment)}" is not a valid \`vertical\` \`alignment\` for a ${this.content.className} widget`);
                }
            } else {
                this.content.setAttribute(textualAlignmentVerticalAttribute, textualAlignmentVerticalDefault);
            }
        } else {
            this.content.setAttribute(textualAlignmentHorizontalAttribute, textualAlignmentHorizontalDefault);
            this.content.setAttribute(textualAlignmentVerticalAttribute, textualAlignmentVerticalDefault);
        }
        if (this.configurationHas(configuration, "color")) {
            /**
             * Valid whether a color is valid
             * @param color String to validate
             * @returns Whether the color is valid
             */
            function isColor(color: string): boolean {
                const style: CSSStyleDeclaration = new Option().style;
                style.color = color;
                return (style.color !== "");
            };
            const color: string = String(this.configurationGet(configuration, "color"));
            if (!isColor(color)) {
                throw this.configurationError(`"${color}" is not a valid \`color\` for a ${this.content.className} widget`);
            }
            const textualShadowRoot: ShadowRoot = this.content.attachShadow({
                mode: "closed"
            });
            const noInheritedStyling: CSSStyleSheet = new CSSStyleSheet();
            noInheritedStyling.replaceSync(`:host{color:${color}!important}`);
            textualShadowRoot.adoptedStyleSheets = [noInheritedStyling];
            const slot: HTMLSlotElement = document.createElement("slot");
            textualShadowRoot.appendChild(slot);
        }
    }
    /**
     * @inheritdoc
     */
    public async prepare(): Promise<void> {
        return Promise.resolve();
    }
};
