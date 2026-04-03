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
    fs.writeFileSync(path.join("dist", bundleName + ".d.ts"), dts.join("\n"));

    await esbuild.build({
        entryPoints: ["ts/index.ts"],
        bundle: true,
        minify: true,
        format: "iife",
        target: "es2019",
        outfile: "dist/" + bundleName + ".js",
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
