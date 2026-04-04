import { loadResource, multimediaResource_t } from "../resources/resource";
import { widget_t } from "./widget";

/**
 * The containment of the video widget
 * @internal
 */
type contain_t = ("fit" | "fill");

/**
 * A video widget
 */
export class video_t extends widget_t {
    constructor() {
        super("video", "video");
    };
    /**
     * @internal
     */
    private source!: string;
    public configuration(configuration: Object): void {
        if (!this.configurationHas(configuration, "source")) {
            this.raiseError("Missing video `source` for a video widget");
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
    };
    public render(): Promise<HTMLElement> {
        return new Promise<HTMLElement>((resolve, reject) => {
            loadResource(this.source).then((resource: multimediaResource_t) => {
                const failure = (): void => {
                    reject(`A video resource of type "${resource.mimeType}" is not supported in this browser`);
                }
                switch ((this.content as HTMLVideoElement).canPlayType(resource.mimeType)) {
                    case "probably":
                        break;
                    case "maybe":
                        console.warn(`The browser cannot guarantee that a resource of type "${resource.mimeType}" is supported`);
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
                (this.content as HTMLVideoElement).oncanplaythrough = () => resolve(this.content);
                this.content.onerror = () => failure();
                (this.content as HTMLVideoElement).load();
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
