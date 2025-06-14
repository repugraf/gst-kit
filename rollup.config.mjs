import resolve from "@rollup/plugin-node-resolve";
import commonjs from "@rollup/plugin-commonjs";
import typescript from "@rollup/plugin-typescript";
import dts from "rollup-plugin-dts";

const config = [
  {
    input: "src/ts/index.ts",
    output: [
      {
        file: "dist/esm/index.mjs",
        format: "esm",
        sourcemap: true,
        exports: "named",
      },
    ],
    plugins: [
      resolve(),
      commonjs(),
      typescript({
        tsconfig: "./tsconfig.json",
        sourceMap: true,
        declaration: true,
        declarationDir: "dist/esm/types",
        outDir: "dist/esm",
      }),
    ],
    external: ["node-addon-api"],
  },
  {
    input: "src/ts/index.ts",
    output: [
      {
        file: "dist/cjs/index.cjs",
        format: "cjs",
        sourcemap: true,
        exports: "named",
      },
    ],
    plugins: [
      resolve(),
      commonjs(),
      typescript({
        tsconfig: "./tsconfig.json",
        sourceMap: true,
        declaration: true,
        declarationDir: "dist/cjs/types",
        outDir: "dist/cjs",
      }),
    ],
    external: ["node-addon-api"],
  },
  {
    input: "src/ts/index.ts",
    output: [{ file: "dist/index.d.ts", format: "esm" }],
    plugins: [
      dts({
        compilerOptions: {
          declaration: true,
          declarationDir: "dist/types",
        },
      }),
    ],
  },
];

export default config;
