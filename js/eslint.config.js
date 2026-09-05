import { sssEslintConfig } from "@sss/sss-guis-eslint";

export default [
    {
        ignores: ["dist/", "*.js"]
    },
    ...sssEslintConfig(),
];
