import esbuild from 'esbuild';
import fs from 'fs';
import path from 'path';
import ts from 'typescript';
import * as sass from 'sass';

/**
 * Compile SCSS file to compressed CSS text
 * @param {string} scssPath
 * @returns {string}
 */
function compileScss(scssPath) {
    const compiler = (sass && sass.default) ? sass.default : sass;
    try {
        if (typeof compiler.compile === 'function') {
            return compiler.compile(scssPath, { style: 'compressed' }).css;
        }
        if (typeof compiler.renderSync === 'function') {
            return compiler.renderSync({ file: scssPath, outputStyle: 'compressed' }).css.toString('utf-8');
        }
        throw new Error('No compatible Sass compile method found in installed sass package');
    } catch (err) {
        throw new Error(`Failed to compile SCSS (${scssPath}): ${err.message}`);
    }
}

/**
 * esbuild plugin to compile SCSS imports into raw CSS text strings
 */
const scssPlugin = {
    name: "scss-text-plugin",
    setup(build) {
        build.onLoad({ filter: /\.scss$/ }, async (args) => {
            return {
                contents: compileScss(args.path),
                loader: "text",
            };
        });
    },
};

/**
 * Extract public non-internal exported value names from an entry file
 * @param {string} entryFile
 * @param {ts.CompilerOptions} compilerOptions
 * @returns {{ program: ts.Program, values: string[] }}
 */
function getPublicExportedValues(entryFile, compilerOptions) {
    const program = ts.createProgram([entryFile], compilerOptions);
    const checker = program.getTypeChecker();
    const sourceFile = program.getSourceFile(entryFile);
    if (!sourceFile) {
        throw new Error(`Source file not found: ${entryFile}`);
    }
    const moduleSymbol = checker.getSymbolAtLocation(sourceFile);
    const exports = checker.getExportsOfModule(moduleSymbol);

    const values = [];
    for (const exp of exports) {
        const decl = exp.valueDeclaration || (exp.declarations && exp.declarations[0]);
        const tags = decl ? ts.getJSDocTags(decl) : [];
        const isInternal = tags.some(t => t.tagName && t.tagName.text === "internal");
        const isValue = Boolean(exp.flags & ts.SymbolFlags.Value);

        if (isValue && !isInternal) {
            values.push(exp.name);
        }
    }
    return { program, values };
}

/**
 * Emit TypeScript declarations using existing program
 * @param {ts.Program} program
 */
function emitDeclarations(program) {
    const emitResult = program.emit();
    const allDiagnostics = ts.getPreEmitDiagnostics(program).concat(emitResult.diagnostics);
    const errors = allDiagnostics.filter(d => d.category === ts.DiagnosticCategory.Error);

    if (errors.length > 0) {
        const formattedErrors = errors.map(diag => {
            const message = ts.flattenDiagnosticMessageText(diag.messageText, "\n");
            if (diag.file && diag.start !== undefined) {
                const { line, character } = diag.file.getLineAndCharacterOfPosition(diag.start);
                return `${diag.file.fileName} (${line + 1}:${character + 1}): ${message}`;
            }
            return message;
        }).join("\n");
        throw new Error(`TypeScript declaration generation failed:\n${formattedErrors}`);
    }
}

async function main() {
    const rootDir = process.cwd();
    const distDir = path.resolve(rootDir, "dist");
    const entryFile = path.resolve(rootDir, "ts/index.ts");

    if (fs.existsSync(distDir)) {
        fs.rmSync(distDir, { recursive: true, force: true });
    }
    fs.mkdirSync(distDir, { recursive: true });

    const tsOptions = {
        declaration: true,
        emitDeclarationOnly: true,
        outDir: distDir,
        target: ts.ScriptTarget.ES2019,
        module: ts.ModuleKind.ESNext,
        moduleResolution: ts.ModuleResolutionKind.NodeJs,
        stripInternal: true,
        skipLibCheck: true,
    };

    // Analyze public exports and generate type declarations
    const { program, values } = getPublicExportedValues(entryFile, tsOptions);
    emitDeclarations(program);

    // Generate ESM runtime accessor stub (index.mjs)
    let stubContent = "// Generated window accessor stub for @sss/sss-guis\n";
    stubContent += "const _guis = (typeof window !== 'undefined' && window.guis) ? window.guis : undefined;\n\n";
    for (const val of values) {
        stubContent += `export const ${val} = _guis?.${val};\n`;
    }
    fs.writeFileSync(path.join(distDir, "index.mjs"), stubContent, "utf8");

    // Bundle standalone runtime IIFE for browser embedding (guis.js)
    const virtualEntryContents = [
        `export { ${values.join(", ")} } from "./ts/index.ts";`,
        'import "./ts/internal/index.ts";',
    ].join("\n");

    await esbuild.build({
        stdin: {
            contents: virtualEntryContents,
            resolveDir: rootDir,
            sourcefile: "runtime-entry.ts",
            loader: "ts",
        },
        bundle: true,
        minify: true,
        sourcemap: false,
        format: "iife",
        globalName: "guis",
        target: "es2019",
        banner: { js: "// SSS-GUIS" },
        plugins: [scssPlugin],
        outfile: path.join(distDir, "guis.js"),
    });
}

main().catch((error) => {
    console.error(error);
    process.exit(1);
});
