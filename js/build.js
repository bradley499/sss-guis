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
            const css = compileScss(args.path);
            return {
                contents: css,
                loader: "text",
            };
        });
    },
};

/**
 * Extract exported symbols from TypeScript source file using TypeScript AST
 * @param {string} filePath
 * @param {string} sourceCode
 * @returns {{ values: string[], types: string[] }}
 */
function extractExportedSymbols(filePath, sourceCode) {
    const sourceFile = ts.createSourceFile(
        filePath,
        sourceCode,
        ts.ScriptTarget.Latest,
        true,
        ts.ScriptKind.TS
    );

    const values = new Set();
    const types = new Set();

    function isExported(node) {
        const modifiers = ts.canHaveModifiers?.(node) ? ts.getModifiers(node) : node.modifiers;
        if (!modifiers) return false;
        return modifiers.some(m => m.kind === ts.SyntaxKind.ExportKeyword);
    }

    function visit(node) {
        if (ts.isClassDeclaration(node) && isExported(node) && node.name) {
            values.add(node.name.text);
        } else if (ts.isFunctionDeclaration(node) && isExported(node) && node.name) {
            values.add(node.name.text);
        } else if (ts.isEnumDeclaration(node) && isExported(node) && node.name) {
            values.add(node.name.text);
        } else if (ts.isVariableStatement(node) && isExported(node)) {
            for (const decl of node.declarationList.declarations) {
                if (ts.isIdentifier(decl.name)) {
                    values.add(decl.name.text);
                }
            }
        } else if (ts.isInterfaceDeclaration(node) && isExported(node) && node.name) {
            types.add(node.name.text);
        } else if (ts.isTypeAliasDeclaration(node) && isExported(node) && node.name) {
            types.add(node.name.text);
        } else if (ts.isExportDeclaration(node)) {
            if (node.exportClause && ts.isNamedExports(node.exportClause)) {
                for (const elem of node.exportClause.elements) {
                    const name = elem.name.text;
                    if (elem.isTypeOnly || node.isTypeOnly) {
                        types.add(name);
                    } else {
                        values.add(name);
                    }
                }
            }
        }

        ts.forEachChild(node, visit);
    }

    visit(sourceFile);

    return {
        values: Array.from(values),
        types: Array.from(types),
    };
}

/**
 * Generate TypeScript declaration files for developer SDK files
 * @param {string[]} tsFiles
 * @param {string} outDir
 */
function generateDeclarations(tsFiles, outDir) {
    const compilerOptions = {
        declaration: true,
        emitDeclarationOnly: true,
        outDir,
        target: ts.ScriptTarget.ES2019,
        module: ts.ModuleKind.ESNext,
        moduleResolution: ts.ModuleResolutionKind.NodeJs,
        stripInternal: true,
        skipLibCheck: true,
    };

    const program = ts.createProgram(tsFiles, compilerOptions);
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
    const startTime = Date.now();
    const rootDir = process.cwd();
    const distDir = path.resolve(rootDir, "dist");
    const tsDir = path.resolve(rootDir, "ts");

    if (fs.existsSync(distDir)) {
        fs.rmSync(distDir, { recursive: true, force: true });
    }
    fs.mkdirSync(distDir, { recursive: true });

    const publicTsFiles = ["ts/resource.ts", "ts/structure.ts", "ts/widget.ts"];
    const virtualEntryContents = [
        ...publicTsFiles.map(f => `export * from \"./${f.replace(/\\/g, '/')}\";`),
        "export * from \"./ts/internal/index.ts\";",
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

    for (const relFile of publicTsFiles) {
        const fullPath = path.resolve(rootDir, relFile);
        const sourceCode = fs.readFileSync(fullPath, "utf8");
        const { values } = extractExportedSymbols(relFile, sourceCode);

        const baseName = path.basename(relFile, ".ts");
        const stubOutPath = path.join(distDir, `${baseName}.mjs`);

        let stubContent = `// Generated window accessor stub for ${baseName}\n`;
        stubContent += "const _guis = (typeof window !== 'undefined' && window.guis) ? window.guis : undefined;\n\n";

        for (const val of values) {
            stubContent += `export const ${val} = _guis?.${val};\n`;
        }

        if (values.length === 0) {
            stubContent += "export {};\n";
        }

        fs.writeFileSync(stubOutPath, stubContent, "utf8");
    }

    generateDeclarations(publicTsFiles, distDir);

    const distFiles = fs.readdirSync(distDir).sort();
    for (const file of distFiles) {
        const stat = fs.statSync(path.join(distDir, file));
    }

    const elapsed = ((Date.now() - startTime) / 1000).toFixed(2);
}

main().catch((error) => {
    console.error(error);
    process.exit(1);
});
