import { loadResource, multimediaResource } from "../resource";

/**
 * Collection of modules with associated promises
 * @internal
 */
const modules: Record<string, Promise<void>> = {};
/**
 * Total amount of modules successfully loaded
 * @internal
 */
let modulesLoaded: number = 0;

/**
 * Asynchronously load a module
 * @async
 * @param url The location of the module
 * @returns Success of the loading of the module
 * @internal
 */
export function loadModule(url: string): Promise<void> {
    if (url in modules) {
        return modules[url];
    }
    const module: Promise<void> = new Promise<void>((resolve: () => void, reject: (reason: Error) => void) => {
        /**
         * Failed to load module
         */
        function failure(): void {
            reject(new Error(`Failed to load module: ${url}`));
        };
        loadResource(url).then((resource: multimediaResource) => {
            const module: HTMLScriptElement = document.createElement("script");
            module.type = "module";
            module.src = resource.blobUrl;
            module.crossOrigin = "anonymous";
            module.async = true;
            module.addEventListener("load", () => {
                modulesLoaded++;
                resolve();
            });
            module.addEventListener("error", () => {
                failure();
            });
            document.head.appendChild(module);
        }).catch((_error: unknown) => {
            failure();
        });
    });
    modules[url] = module;
    return module;
}
/**
 * Checks whether any modules are yet to be successfully loaded
 * @returns Whether modules are yet to be successfully loaded
 * @internal
 */
function loadingModules(): boolean {
    return (modulesLoaded != Object.keys(modules).length);
}
/**
 * Wait for all modules to load
 * @returns Success of loading all of the modules
 * @internal
 */
export function loadModules(): Promise<void> {
    return new Promise<void>((resolve: () => void, reject: (reason: Error) => void) => {
        /**
         * Handles errors during the module loading process
         * @param error The error event
         */
        function moduleLoadingError(error: ErrorEvent): void {
            error.preventDefault();
            reject(new Error(`Module error: ${error.message}`));
        }
        window.addEventListener("error", moduleLoadingError);
        void (async (): Promise<void> => {
            try {
                do {
                    void await Promise.all(Object.values(modules));
                } while (loadingModules());
                resolve();
            } catch (error: unknown) {
                reject(error instanceof Error ? error : new Error(String(error)));
            } finally {
                window.removeEventListener("error", moduleLoadingError);
            }
        })();
    });
}
