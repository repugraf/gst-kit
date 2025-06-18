#!/usr/bin/env node
import { Pipeline } from "../dist/esm/index.mjs";

const pipeline = new Pipeline("appsrc name=mysource is-live=true ! videoconvert ! autovideosink");
const appsrc = pipeline.getElementByName("mysource");

appsrc.setElementProperty(
  "caps",
  "video/x-raw,format=RGB,width=320,height=240,bpp=24,depth=24,framerate=0/1"
);

pipeline.play();

setInterval(
  () =>
    appsrc.push(
      Buffer.alloc(320 * 240 * 3, Math.floor(Math.random() * 16777215).toString(16), "hex")
    ),
  33
);
