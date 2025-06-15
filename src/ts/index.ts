import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { createRequire } from "node:module";

// Get the directory name of the current module
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// Get the project root directory (two levels up from __dirname)
const projectRoot = join(__dirname, "../../");

type Element = {
  readonly type: "element";
};

type AppSinkElement = {
  readonly type: "app-sink-element";
  pull(timeout?: number): Promise<Buffer | null>;
};

type AppSrcElement = {
  readonly type: "app-src-element";
};

interface Pipeline {
  play(): void;
  stop(): void;
  playing(): boolean;
  getElementByName(name: string): Element | AppSinkElement | AppSrcElement | null;
}

// Define the interface for the native addon
interface NativeAddon {
  Pipeline: new (pipeline: string) => Pipeline;
}

// Create require function for ESM
const require = createRequire(import.meta.url);

// Load the native addon
const nativeAddon: NativeAddon = require(join(projectRoot, "build/Release/native_addon.node"));

const { Pipeline: PipelineClass } = nativeAddon;

export { PipelineClass as Pipeline };

export default nativeAddon;
