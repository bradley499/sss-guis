import splashStyling from "./splash.scss";

import { getGui, guiSchema } from "./gui";
import { structureGenerate, structureLoadEvent, structureLoadEventType } from "../structure";
import { widget } from "../widget";
import { loadStylesheet } from "./stylesheet";
import { loadModule, loadModules } from "./module";
import "./grid";

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
        let gui: guiSchema;
        try {
            gui = getGui();
            projectName = gui.project;
            splashHeading.innerText = projectName;
        } catch (error: unknown) {
            throw error instanceof Error ? error : new Error(String(error));
        }

        // Start loading stylesheet
        const stylesheet: Promise<void> = loadStylesheet(gui.stylesheet);

        // Load modules
        splashStatus.innerText = "Loading modules...";
        void gui.modules.map((module: string) => loadModule(module));
        await loadModules();

        // Load layouts
        splashStatus.innerText = "Loading layout...";
        const [mainWidget]: [widget, unknown] = await Promise.all([
            structureGenerate(gui.structure),
            stylesheet,
        ]);

        if (mainWidget instanceof widget) {
            splashStatus.innerText = "Rending layout...";
            const mainElement: HTMLElement = await mainWidget.render();

            while (document.body.children.length > 0) {
                document.body.removeChild(document.body.children[0]);
            }
            document.documentElement.replaceChild(document.createElement("body"), splashContainer);
            document.body.appendChild(mainElement);
            document.title = gui.name.trim();

            document.dispatchEvent(new CustomEvent<structureLoadEvent>(structureLoadEventType, {
                detail: {
                    success: true,
                    message: undefined
                }
            }));
        }
    } catch (error: unknown) {
        const message: string = error instanceof Error ? error.message : String(error);
        setError(message);
        document.dispatchEvent(new CustomEvent<structureLoadEvent>(structureLoadEventType, {
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
