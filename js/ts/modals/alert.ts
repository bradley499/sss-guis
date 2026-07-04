import { dialog } from "./dialog";

/**
 * Show an alert dialog
 * @param message The message to show in the alert
 * @returns Whether the alert has been interacted with by a user
 */
export function alert(message: string): Promise<boolean> {
    return new Promise<boolean>((resolve: (value: boolean) => void, reject: (reason: Error) => void) => {
        const content: HTMLParagraphElement = document.createElement("p");
        content.innerText = message;
        dialog(null, content, ["Close"]).then((_button: string) => {
            resolve(true);
        }).catch((_button: unknown) => {
            reject(Error());
        });
    });
}
