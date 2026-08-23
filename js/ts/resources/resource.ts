/**
 * Interface for the multimedia resource
 */
export interface multimediaResource_t {
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
const resources: Record<string, Promise<multimediaResource_t>> = {};

/**
 * Asynchronously load a resource
 * @async
 * @param url The location of the resource
 * @returns The loaded resource
 */
export function loadResource(url: string): Promise<multimediaResource_t> {
    if (url in resources) {
        return resources[url];
    }
    const blob: Promise<multimediaResource_t> = new Promise<multimediaResource_t>((resolve: (value: multimediaResource_t) => void, reject: (reason: Error) => void) => {
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
            const resource: multimediaResource_t = {
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
