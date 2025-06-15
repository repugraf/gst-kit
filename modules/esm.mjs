import assert from "node:assert";
import gst from "../dist/esm/index.mjs";

assert(!!gst, "ESM: Module is not defined");
assert(gst.Pipeline, "ESM: Pipeline is not defined");
