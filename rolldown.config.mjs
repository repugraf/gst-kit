import { defineConfig } from "rolldown";
import { dts } from "rolldown-plugin-dts";

const external = ["node-addon-api", /^node:/];
const input = "src/ts/index.ts";

export default defineConfig([
  {
    input,
    output: {
      dir: "dist/esm",
      entryFileNames: "index.mjs",
      format: "esm",
      sourcemap: true,
      exports: "named",
      codeSplitting: false,
    },
    external,
  },
  {
    input,
    output: {
      dir: "dist/cjs",
      entryFileNames: "index.cjs",
      format: "cjs",
      sourcemap: true,
      exports: "named",
      codeSplitting: false,
    },
    external,
  },
  {
    input,
    output: {
      dir: "dist",
      format: "esm",
    },
    plugins: [dts({ emitDtsOnly: true })],
    external,
  },
]);
