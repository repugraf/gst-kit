#!/usr/bin/env node
import { Pipeline } from "../dist/esm/index.mjs";

// Demonstrates sending an EOS event directly to the pipeline.
// This works with any source type (hardware cameras, network streams, test sources)
// unlike appsrc.endOfStream() which only works on appsrc elements.

const pipeline = new Pipeline("videotestsrc ! videoconvert ! autovideosink");

await pipeline.play();
console.log("▶️  Pipeline playing. Sending EOS in 5 seconds...");

setTimeout(() => {
  console.log("🏁 Sending End-of-Stream to pipeline now!");
  const sent = pipeline.endOfStream();
  console.log(`   endOfStream() returned: ${sent}`);
}, 5000);

while (true) {
  const message = await pipeline.busPop(100);
  if (!message) continue;
  if (message.type === "eos") {
    await pipeline.stop();
    break;
  }
}

console.log("🎉 End-of-Stream received! Pipeline stopped cleanly.");
