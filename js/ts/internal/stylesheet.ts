import { loadResource, multimediaResource } from "../resource";

/**
 * Asynchronously load a stylesheet
 * @async
 * @param url The location of the stylesheet
 * @returns Success of the loading of the stylesheet
 * @internal
 */
export function loadStylesheet(url: string): Promise<void> {
    return new Promise<void>((resolve: () => void, reject: (reason: Error) => void) => {
        /**
         * Failed to load stylesheet
         */
        function failure(): void {
            reject(new Error(`Failed to load stylesheet: ${url}`));
        };
        loadResource(url).then((resource: multimediaResource): void => {
            const stylesheet: HTMLLinkElement = document.createElement("link");
            stylesheet.rel = "stylesheet";
            stylesheet.type = resource.mimeType;
            stylesheet.href = resource.blobUrl;
            stylesheet.addEventListener("load", () => {
                resolve();
            });
            stylesheet.addEventListener("error", () => {
                failure();
            });
            document.head.appendChild(stylesheet);
        }).catch((_error: unknown) => {
            failure();
        });
    });
}
