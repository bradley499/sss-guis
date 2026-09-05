/**
 * Interface for the multimedia resource
 */
export interface multimediaResource {
    /**
     * The URL to access the resource at
     */
    blobUrl: string;
    /**
     * The MIME type of the resource
     */
    mimeType: string;
}

/**
 * Collection of resources with associated promises
 * @internal
 */
const resources: Record<string, Promise<multimediaResource>> = {};

/**
 * Asynchronously load a resource
 * @async
 * @param url The location of the resource
 * @returns The loaded resource
 */
export function loadResource(url: string): Promise<multimediaResource> {
    if (url in resources) {
        return resources[url];
    }
    const blob: Promise<multimediaResource> = new Promise<multimediaResource>((resolve: (value: multimediaResource) => void, reject: (reason: Error) => void) => {
        /**
         * Failed to load multimedia resource
         */
        function failure(): void {
            reject(new Error(`Failed to load multimedia resource: ${url}`));
        };
        fetch(url).then(async (response: Response): Promise<void> => {
            if (!response.ok) {
                failure();
            }
            const blob: Blob = await response.blob();
            const resource: multimediaResource = {
                blobUrl: URL.createObjectURL(blob),
                mimeType: blob.type
            };
            window.addEventListener("unload", () => {
                URL.revokeObjectURL(resource.blobUrl);
            });
            resolve(resource);
        }).catch((_error: unknown) => {
            failure();
        });
    });
    resources[url] = blob;
    return blob;
}
