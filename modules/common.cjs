const assert = require("node:assert");
const gst = require("../dist/cjs/index.cjs");

assert(!!gst, "CommonJS: Module is not defined");
assert(gst.Pipeline, "CommonJS: Pipeline is not defined");
