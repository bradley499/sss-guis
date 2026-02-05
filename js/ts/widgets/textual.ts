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
    public setConfiguration(configuration: Object, textualAlignmentHorizontalDefault: textualAlignmentHorizontal_t, textualAlignmentVerticalDefault: textualAlignmentVertical_t): void {
        if (!this.configurationHas(configuration, "text")) {
            throw new Error(`A ${this.content.className} widget requires \`text\` to be shown`);
        }
        this.content.innerText = (configuration as any).text as string;
        if (this.configurationHas(configuration, "alignment")) {
            const alignment: any = (configuration as any).alignment;
            if (this.configurationHas(alignment, "horizontal")) {
                const horizontalAlignment: textualAlignmentHorizontal_t = alignment.horizontal;
                switch (horizontalAlignment) {
                    case "left":
                    case "right":
                    case "center":
                        this.content.setAttribute(textualAlignmentHorizontalAttribute, horizontalAlignment);
                        break;
                    default:
                        throw new Error(`"${horizontalAlignment}" is not a valid horizontal alignment for a ${this.content.className} widget`);
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
                        throw new Error(`"${verticalAlignment}" is not a valid vertical alignment for a ${this.content.className} widget`);
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
             * @param {string} color String to validate
             * @returns {boolean} Whether the color is valid
             */
            const isColor = (color: string): boolean => {
                const style: CSSStyleDeclaration = new Option().style;
                style.color = color;
                return (style.color !== "");
            }
            const color: string = (configuration as any).color;
            if (!isColor(color)) {
                throw new Error(`"${color}" is not a valid color for a ${this.content.className} widget`);
            }
            const textualShadowRoot: ShadowRoot = this.content.attachShadow({ mode: "closed" });
            const noInheritedStyling: CSSStyleSheet = new CSSStyleSheet();
            noInheritedStyling.replaceSync(`:host{color:${color}!important}`);
            textualShadowRoot.adoptedStyleSheets = [noInheritedStyling];
            const slot = document.createElement("slot");
            textualShadowRoot.appendChild(slot);
        }
    }
    public render(): Promise<HTMLElement> {
        return new Promise<HTMLElement>((resolve, _reject) => {
            resolve(this.content);
        });
    };
};
