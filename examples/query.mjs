#!/usr/bin/env node
import { Pipeline } from "../dist/esm/index.mjs";

const pipeline = new Pipeline("videotestsrc ! videoconvert ! autovideosink");
pipeline.play();

setInterval(
  () => console.log("Query return:", pipeline.queryPosition(), "/", pipeline.queryDuration()),
  500
);
