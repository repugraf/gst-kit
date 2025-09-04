import { defineConfig } from "vitest/config";

export default defineConfig({
  test: {
    globals: true,
    environment: "node",
    include: ["src/ts/**/*.test.ts"],
    coverage: {
      provider: "v8",
      reporter: ["text", "json", "html"],
      exclude: ["node_modules/", "dist/"],
    },
    slowTestThreshold: 2000,
    testTimeout: 20000,
    env: {
      NODE_ENV: "test",
    },
    deps: {
      optimizer: {
        ssr: {
          include: ["node-addon-api"],
        },
      },
    },
  },
});
