import { existsSync } from "node:fs";
import { join, dirname } from "node:path";
import { execSync } from "node:child_process";
import { fileURLToPath } from "node:url";

// Get the directory name of the current module
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// Get the project root directory (one level up from __dirname)
const projectRoot = join(__dirname, "..");
const addonPath = join(projectRoot, "build/Release/native_addon.node");

if (!existsSync(addonPath)) {
  console.log("Native addon not found, building...");
  execSync("npm run build", { stdio: "inherit", cwd: projectRoot });
}
