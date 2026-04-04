import { loadResource, multimediaResource_t } from "../resources/resource";
import { widget_t } from "./widget";

/**
 * The containment of the image widget
 * @internal
 */
type contain_t = ("fit" | "fill");

/**
 * An image widget
 */
export class image_t extends widget_t {
    constructor() {
        super("img", "image");
    };
    /**
     * @internal
     */
    private source!: string;
    public configuration(configuration: Object): void {
        if (!this.configurationHas(configuration, "source")) {
            this.raiseError("Missing image `source` for an image widget");
        }
        this.source = (configuration as any).source as string;
        if (this.configurationHas(configuration, "contain")) {
            const contain: contain_t = (configuration as any).contain;
            switch (contain) {
                case "fit":
                case "fill":
                    this.content.setAttribute("contain", contain);
                    break;
                default:
                    this.raiseError(`"${contain}" is not a valid \`contain\` property for a video widget`);
            }
        }
    }
    public render(): Promise<HTMLElement> {
        return new Promise<HTMLElement>((resolve, reject) => {
            loadResource(this.source).then((resource: multimediaResource_t) => {
                this.content.onload = () => resolve(this.content);
                this.content.onerror = () => reject(`An image resource of type "${resource.mimeType}" is not supported in this browser`);
                (this.content as HTMLImageElement).src = resource.blobUrl;
            }).catch((error) => {
                try {
                    this.raiseError(error)
                } catch (error) {
                    reject(error);
                }
            });
        });
    };
};
