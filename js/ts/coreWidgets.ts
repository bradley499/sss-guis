import { structureDeclareWidget } from "./structure";
import { audio_t } from "./widgets/audio";
import { banner_t } from "./widgets/banner";
import { container_t } from "./widgets/container";
import { image_t } from "./widgets/image";
import { layout_t } from "./widgets/layout";
import { tabs_t } from "./widgets/tabs";
import { text_t } from "./widgets/text";
import { video_t } from "./widgets/video";
import { void_t } from "./widgets/void";

/**
 * Register the core widgets
 * @internal
 */
export function registerCoreWidgets(): void {
    structureDeclareWidget("null", void_t);
    structureDeclareWidget("layout", layout_t);
    structureDeclareWidget("container", container_t);
    structureDeclareWidget("tabs", tabs_t);
    structureDeclareWidget("banner", banner_t);
    structureDeclareWidget("text", text_t);
    structureDeclareWidget("image", image_t);
    structureDeclareWidget("video", video_t);
    structureDeclareWidget("audio", audio_t);
}
