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
    ], {
        preferredConfigPath: path.join(process.cwd(), "tsconfig.json")
    });

    if (!fs.existsSync("dist")) {
        fs.mkdirSync("dist");
    }

    fs.writeFileSync(path.join("dist", bundleName + ".d.ts"), dts.join("\n"));

    const commonOptions = {
        bundle: true,
        minify: true,
        target: "es2019",
        banner: { js: "// SSS-GUIS" },
        plugins: [{
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
        }],
    };

    await esbuild.build({
        ...commonOptions,
        entryPoints: ["ts/index.ts"],
        format: "iife",
        outfile: "dist/" + bundleName + ".js",
    });

    await esbuild.build({
        ...commonOptions,
        entryPoints: ["ts/exported.ts"],
        format: "esm",
        outfile: "dist/" + bundleName + ".mjs",
    });
}

build().catch(error => {
    console.error(error);
    process.exit(1);
});
