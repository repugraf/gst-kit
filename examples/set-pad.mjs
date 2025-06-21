#!/usr/bin/env node
import { Pipeline } from "../dist/esm/index.mjs";

const pipeline = new Pipeline(
  "input-selector name=sel ! autovideosink videotestsrc pattern=0 ! sel.sink_0 videotestsrc pattern=1 ! sel.sink_1"
);

const sel = pipeline.getElementByName("sel");
pipeline.play();

let t = 0;
setInterval(() => {
  console.log("t: %d", ++t);
  sel.setPad("active-pad", t % 2 === 0 ? "sink_0" : "sink_1");
}, 1000);
