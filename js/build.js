import esbuild from "esbuild";
import sass from "sass";
import { execSync } from "child_process";
import { generateDtsBundle } from "dts-bundle-generator";
import path from "path";
import fs from "fs";

const bundleName = "guis";

async function build() {
    execSync("npx tsc --noEmit", { stdio: "inherit" });
    const dts = generateDtsBundle([
        {
            filePath: "ts/exported.ts",
            output: { noBanner: true },
        },
    ]);

    if (!fs.existsSync("dist")) {
        fs.mkdirSync("dist");
    }
    const modifiedDts = dts.join("\n").replace(/^export\s+(declare\s+)?function\s+/gm, "declare function ").replace(/^(export\s+)?interface\s+/gm, "declare interface ").replace(/^export\s+(declare\s+)?type\s+/gm, "declare type ").replace(/^export\s+(declare\s+)?(abstract\s+)?class\s+/gm, (match) => {
        // Remove "export " but keep "declare " and "abstract "
        return match.replace("export ", "").trimStart().startsWith("declare")
            ? match.replace("export ", "")
            : "declare " + match.replace("export ", "");
    }).replace(/^export\s*\{[\s\S]*?\};?\s*$/gm, "").replace(/^\{\s*\};?\s*$/gm, "").trim() + "\n";
    fs.writeFileSync(path.join("dist", bundleName + ".d.ts"), modifiedDts);

    await esbuild.build({
        entryPoints: ["ts/index.ts"],
        bundle: true,
        minify: true,
        format: "iife",
        target: "es2019",
        outfile: "dist/" + bundleName + ".js",
        banner: {
            js: "// SSS-GUIS"
        },
        plugins: [
            {
                name: "scss-string",
                setup(build) {
                    build.onLoad({ filter: /\.scss$/ }, (args) => {
                        const result = sass.renderSync({
                            file: args.path,
                            outputStyle: "compressed"
                        });
                        return {
                            contents: `export default ${JSON.stringify(result.css.toString())};`,
                            loader: "js"
                        };
                    });
                }
            }
        ],
    });
}

build().catch(error => {
    console.error(error);
    process.exit(1);
});
