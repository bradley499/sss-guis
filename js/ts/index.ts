// @ts-expect-error: SCSS imports are handled by the custom esbuild plugin at build time.
import splashStyling from "./splash.scss";

import { gui_t } from "./gui";
import { structureGenerate } from "./structure";
import { widget_t } from "./widgets/widget";
import { loadStylesheet } from "./resources/stylesheet";
import { exportToWindow, structureLoadEvent, structureLoadEventType } from "./exported";
import { loadModule, loadModules } from "./resources/module";
import "./coreWidgets";

/**
 * Asynchronously start SSS GUI generation
 * @internal
 */
async function main(): Promise<void> {
    let projectName: string = "SSS";
    // Splash screen
    const splashContainer: HTMLBodyElement = document.createElement("body");
    const splashShadowRoot: ShadowRoot = splashContainer.attachShadow({
        "mode": "closed"
    });
    const noInheritedStyling: CSSStyleSheet = new CSSStyleSheet();
    noInheritedStyling.replaceSync("*{all:initial;display:block;}" + (splashStyling as string));
    splashShadowRoot.adoptedStyleSheets = [noInheritedStyling];
    const splashContent: HTMLDivElement = document.createElement("div");
    const splashHeading: HTMLHeadingElement = document.createElement("h1");
    splashHeading.innerText = projectName;
    const splashStatus: HTMLParagraphElement = document.createElement("p");
    splashStatus.innerText = "Loading...";
    /**
     * Put the document into a error state
     * @param error Error to show
     */
    const setError = (error: string): void => {
        splashStatus.innerText = error;
        splashStatus.classList.add("error");
        document.title = `Error | ${projectName}`;
    };
    splashContent.appendChild(splashHeading);
    splashContent.appendChild(splashStatus);
    splashShadowRoot.appendChild(splashContent);
    document.body.replaceWith(splashContainer);

    try {
        let gui_data: gui_t;
        try {
            gui_data = new gui_t();
            projectName = gui_data.project;
            splashHeading.innerText = projectName;
        } catch (error: unknown) {
            throw error instanceof Error ? error : new Error(String(error));
        }

        exportToWindow();

        // Start loading stylesheet
        const stylesheet: Promise<void> = loadStylesheet(gui_data.stylesheet);

        // Load modules
        splashStatus.innerText = "Loading modules...";
        void gui_data.modules.map((module: string) => loadModule(module));
        await loadModules();

        // Load layouts
        splashStatus.innerText = "Loading layout...";
        const [mainWidget]: [widget_t, unknown] = await Promise.all([
            structureGenerate(gui_data.structure),
            stylesheet,
        ]);

        if (mainWidget instanceof widget_t) {
            splashStatus.innerText = "Rending layout...";
            const mainElement: HTMLElement = await mainWidget.render();

            while (document.body.children.length > 0) {
                document.body.removeChild(document.body.children[0]);
            }
            document.documentElement.replaceChild(document.createElement("body"), splashContainer);
            document.body.appendChild(mainElement);
            document.title = gui_data.name.trim();

            document.dispatchEvent(new CustomEvent<structureLoadEvent>(structureLoadEventType(), {
                detail: {
                    success: true,
                    message: undefined
                }
            }));
        }
    } catch (error: unknown) {
        const message: string = error instanceof Error ? error.message : String(error);
        setError(message);
        document.dispatchEvent(new CustomEvent<structureLoadEvent>(structureLoadEventType(), {
            detail: {
                success: false,
                message: message,
            }
        }));
    }
}

window.addEventListener("load", () => {
    void main();
});
