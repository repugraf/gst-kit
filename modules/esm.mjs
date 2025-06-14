import assert from "node:assert";
import { add } from "../dist/esm/index.mjs";

assert(add(1, 2) === 3, "ESM: Addition failed");
