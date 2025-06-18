#!/usr/bin/env node
import { Pipeline } from "../dist/esm/index.mjs";

const pipeline = new Pipeline("videotestsrc num-buffers=15 ! appsink name=sink");
const appsink = pipeline.getElementByName("sink");

pipeline.play();

while (true) {
  const sample = await appsink.getSample();

  if (sample?.buffer) {
    console.log("BUFFER size", sample.buffer.length);
  } else {
    console.log("NULL BUFFER");
    await new Promise(r => setTimeout(r, 500));
  }
}
