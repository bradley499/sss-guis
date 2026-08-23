import { loadResource, multimediaResource_t } from "../resources/resource";
import { structureDeclareWidget } from "../structure";
import { widget_t } from "./widget";

/**
 * The containment of the video widget
 * @internal
 */
type contain_t = ("fit" | "fill");

/**
 * A video widget
 */
export class video_t extends widget_t<HTMLVideoElement> {
    /**
     * @internal
    */
    private source!: string;
    /**
     * @inheritdoc
     */
    constructor() {
        super("video");
    }
    /**
     * @inheritdoc
     */
    public configuration(configuration: object): void {
        if (!this.configurationHas(configuration, "source")) {
            throw this.configurationError("Missing video `source` for a video widget");
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
                /**
                 * Failed to load video resource
                 */
                function failure(): void {
                    reject(Error(`A video resource of type "${resource.mimeType}" is not supported in this browser`));
                }
                switch (this.content.canPlayType(resource.mimeType)) {
                    case "probably":
                        break;
                    case "maybe":
                        console.warn(`The browser cannot guarantee that a resource of type "${resource.mimeType}" is supported (${this.source})`);
                        break;
                    default:
                        failure();
                        break;
                }
                this.content.setAttribute("controls", "controls");
                const source: HTMLSourceElement = document.createElement("source");
                source.src = resource.blobUrl;
                source.type = resource.mimeType;
                this.content.appendChild(source);
                this.content.addEventListener("canplaythrough", (): void => {
                    resolve();
                });
                this.content.addEventListener("error", (): void => {
                    failure();
                });
                this.content.load();
            }).catch((error: unknown) => {
                reject(this.configurationError(error));
            });
        });
    }
};

structureDeclareWidget("video", video_t);
