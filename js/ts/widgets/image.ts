import { loadResource, multimediaResource_t } from "../resources/resource";
import { structureDeclareWidget } from "../structure";
import { widget_t } from "./widget";

/**
 * The containment of the image widget
 * @internal
 */
type contain_t = ("fit" | "fill");

/**
 * An image widget
 */
export class image_t extends widget_t<HTMLImageElement> {
    /**
     * @internal
    */
    private source!: string;
    /**
     * @inheritdoc
     */
    constructor() {
        super("img");
    }
    /**
     * @inheritdoc
     */
    public configuration(configuration: object): void {
        if (!this.configurationHas(configuration, "source")) {
            throw this.configurationError("Missing image `source` for an image widget");
        }
        this.source = (this.configurationGet(configuration, "source") as string);
        if (this.configurationHas(configuration, "contain")) {
            const contain: contain_t = (this.configurationGet(configuration, "contain") as contain_t);
            switch (contain) {
                case "fit":
                case "fill":
                    this.content.setAttribute("contain", contain);
                    break;
                default:
                    throw this.configurationError(`"${String(contain)}" is not a valid \`contain\` property for a video widget`);
            }
        }
    }
    /**
     * @inheritdoc
     */
    public async prepare(): Promise<void> {
        return new Promise<void>((resolve: () => void, reject: (reason: Error) => void) => {
            loadResource(this.source).then((resource: multimediaResource_t) => {
                this.content.addEventListener("load", () => {
                    resolve();
                });
                this.content.addEventListener("error", () => {
                    reject(this.configurationError(`An image resource of type "${resource.mimeType}" is not supported in this browser`));
                });
                this.content.src = resource.blobUrl;
            }).catch((error: unknown) => {
                reject(this.configurationError(error));
            });
        });
    };
};

structureDeclareWidget("image", image_t);
