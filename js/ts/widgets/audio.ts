import { loadResource, multimediaResource_t } from "../resources/resource";
import { widget_t } from "./widget";

/**
 * An audio widget
 */
export class audio_t extends widget_t {
    constructor() {
        super("audio", "audio");
    };
    /**
     * @internal
     */
    protected source!: string;
    public configuration(configuration: Object): void {
        if (!this.configurationHas(configuration, "source")) {
            this.raiseError("Missing audio `source` for an audio widget");
        }
        this.source = (configuration as any).source as string;
    }
    public render(): Promise<HTMLElement> {
        return new Promise<HTMLElement>((resolve, reject) => {
            loadResource(this.source).then((resource: multimediaResource_t) => {
                const failure = (): void => {
                    reject(`An audio resource of type "${resource.mimeType}" is not supported in this browser`);
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
                (this.content as HTMLAudioElement).oncanplaythrough = () => resolve(this.content);
                this.content.onerror = () => failure();
                (this.content as HTMLAudioElement).load();
                resolve(this.content);
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
