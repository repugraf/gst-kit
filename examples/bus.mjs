#!/usr/bin/env node
import { Pipeline } from "../dist/esm/index.mjs";

const pipeline = new Pipeline("videotestsrc num-buffers=60 ! autovideosink");

pipeline.play();

while (true) {
  const msg = await pipeline.busPop();
  console.log(msg);
  if (msg?.type === "eos") {
    console.log("End of Stream");
    pipeline.stop();
    break;
  }
}
