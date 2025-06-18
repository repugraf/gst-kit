#!/usr/bin/env node
import { Pipeline } from "../dist/esm/index.mjs";

const pipeline = new Pipeline("videotestsrc ! fakesink enable-last-sample=true name=sink");
const fakesink = pipeline.getElementByName("sink");

pipeline.play();

setTimeout(() => {
  const sample = fakesink.getElementProperty("last-sample");
  console.log("Got sample of: ", sample.buffer.length, "bytes. With caps: ", sample.caps);
  pipeline.stop();
}, 1000);
