#!/usr/bin/env node
import { Pipeline } from "../dist/esm/index.mjs";

const pipeline = new Pipeline("videotestsrc ! timeoverlay ! autovideosink");
pipeline.play();

setInterval(() => {
  console.log(`@ ${pipeline.queryPosition()}s / ${pipeline.queryDuration()}s, seeking to 1m`);
  pipeline.seek(60);
}, 5000);
