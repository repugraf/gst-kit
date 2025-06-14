const assert = require("node:assert");
const {Pipeline} = require("../dist/cjs/index.cjs");

assert(!!Pipeline, "CommonJS: Pipeline is not defined");
