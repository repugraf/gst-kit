import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { createRequire } from "node:module";

// Get the directory name of the current module
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// Get the project root directory (two levels up from __dirname)
const projectRoot = join(__dirname, "../../");

export type GStreamerPropertyValue = string | number | boolean;

// Sample object returned for GST_VALUE_HOLDS_SAMPLE properties
export type GStreamerSample = {
  buf?: Buffer;
  caps: {
    name?: string;
    // Additional structure fields (format, width, height, framerate, etc.)
    [key: string]: GStreamerPropertyValue | undefined;
  };
};

// Extended return types including arrays, buffers, and samples
export type GStreamerPropertyReturnValue =
  | GStreamerPropertyValue
  | GStreamerPropertyValue[]
  | Buffer
  | GStreamerSample
  | null;

type ElementBase = {
  getElementProperty: (key: string) => GStreamerPropertyReturnValue;
  setElementProperty: (key: string, value: GStreamerPropertyValue) => void;
};

type Element = {
  readonly type: "element";
} & ElementBase;

type AppSinkElement = {
  readonly type: "app-sink-element";
  pull(timeout?: number): Promise<Buffer | null>;
} & ElementBase;

type AppSrcElement = {
  readonly type: "app-src-element";
} & ElementBase;

interface Pipeline {
  play(): void;
  stop(): void;
  playing(): boolean;
  getElementByName(name: string): Element | AppSinkElement | AppSrcElement | null;
}

// Define the interface for the native addon
interface NativeAddon {
  Pipeline: new (pipeline: string) => Pipeline;
  GStreamerPropertyValue: GStreamerPropertyValue;
  GStreamerSample: GStreamerSample;
  GStreamerPropertyReturnValue: GStreamerPropertyReturnValue;
}

// Create require function for ESM
const require = createRequire(import.meta.url);

// Load the native addon
const nativeAddon: NativeAddon = require(join(projectRoot, "build/Release/native_addon.node"));

const { Pipeline: PipelineClass } = nativeAddon;

export { PipelineClass as Pipeline };

export default nativeAddon;
