#!/usr/bin/env node
import { Pipeline } from "../dist/esm/index.mjs";

/** encoding and decoding */
const pipeline = new Pipeline(
  "videotestsrc num-buffers=100 ! videoconvert ! x264enc ! rtph264pay ! rtph264depay name=depay ! h264parse ! avdec_h264 ! videoconvert ! appsink name=sink"
);
const appsink = pipeline.getElementByName("sink");

await pipeline.play();

const depay = pipeline.getElementByName("depay");

while (true) {
  const sample = await appsink.getSample();

  if (sample?.buffer) {
    const stats = depay.getElementProperty("stats");
    console.log(`Frame size = ${sample.buffer.length}; rtp-timestamp = ${stats?.timestamp}`);
  } else {
    console.log("No Frame received. Stopping pipeline...");
    break;
  }
}

await pipeline.stop();
