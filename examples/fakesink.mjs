#!/usr/bin/env node
import { Pipeline } from "../dist/esm/index.mjs";

const pipeline = new Pipeline("videotestsrc ! fakesink enable-last-sample=true name=sink");
const fakesink = pipeline.getElementByName("sink");

pipeline.play();

setTimeout(() => {
  const sampleResult = fakesink.getElementProperty("last-sample");
  const sample = sampleResult?.value;
  console.log("Got sample of: ", sample.buffer.length, "bytes. With caps: ", sample.caps);
  pipeline.stop();
}, 1000);
