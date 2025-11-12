import { rmSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";

// Get the directory name of the current module
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

const rootDir = resolve(join(__dirname, ".."));

const buildDir = join(rootDir, "build");
const distDir = join(rootDir, "dist");

console.log("Cleaning build and dist directories...");

rmSync(buildDir, { recursive: true, force: true });
rmSync(distDir, { recursive: true, force: true });

console.log("Clean complete.");
