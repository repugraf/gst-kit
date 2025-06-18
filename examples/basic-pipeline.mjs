#!/usr/bin/env node
import { Pipeline } from "../dist/esm/index.mjs";

const pipeline = new Pipeline(
  "videotestsrc ! capsfilter name=filter ! textoverlay name=text ! autovideosink"
);
const filter = pipeline.getElementByName("filter");
const target = pipeline.getElementByName("text");

pipeline.play();

target.setElementProperty("text", "hello");
target.setElementProperty("font-desc", "Helvetica 32");
filter.setElementProperty("caps", "video/x-raw,width=1280,height=720");

let t = 0;
setInterval(() => {
  t++;
  target.setElementProperty("text", "@" + t);
}, 1000);
