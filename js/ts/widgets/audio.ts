import { loadResource, multimediaResource_t } from "../resources/resource";
import { widget_t } from "./widget";

/**
 * An audio widget
 */
export class audio_t extends widget_t {
    protected override content!: HTMLAudioElement;
    /**
     * @internal
    */
    protected source!: string;
    /**
     * @inheritdoc
     */
    constructor() {
        super("audio", "audio");
    }
    /**
     * @inheritdoc
     */
    public configuration(configuration: object): void {
        if (!this.configurationHas(configuration, "source")) {
            throw this.configurationError("Missing audio `source` for an audio widget");
        }
        this.source = (this.configurationGet(configuration, "source") as string);
    }
    /**
     * @inheritdoc
     */
    public async prepare(): Promise<void> {
        return new Promise<void>((resolve: () => void, reject: (reason: Error) => void) => {
            loadResource(this.source).then((resource: multimediaResource_t) => {
                /**
                 * Failed to audio resource
                 */
                function failure(): void {
                    reject(Error(`An audio resource of type "${resource.mimeType}" is not supported in this browser`));
                }
                switch ((this.content as HTMLVideoElement).canPlayType(resource.mimeType)) {
                    case "probably":
                        break;
                    case "maybe":
                        console.warn(`The browser cannot guarantee that a resource of type "${resource.mimeType}" is supported (${this.source})`);
                        break;
                    default:
                        failure();
                }
                this.content.setAttribute("controls", "controls");
                const source: HTMLSourceElement = document.createElement("source");
                source.src = resource.blobUrl;
                source.type = resource.mimeType;
                this.content.appendChild(source);
                this.content.addEventListener("canplaythrough", () => {
                    resolve();
                });
                this.content.addEventListener("error", () => {
                    failure();
                });
                this.content.load();
            }).catch((error: unknown) => {
                reject(this.configurationError(error));
            });
        });
    }
};
