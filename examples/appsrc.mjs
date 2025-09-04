#!/usr/bin/env node
import { Pipeline } from "../dist/esm/index.mjs";
import { randomBytes } from "node:crypto";

const pipeline = new Pipeline("appsrc name=mysource is-live=true ! videoconvert ! autovideosink");
const appsrc = pipeline.getElementByName("mysource");

appsrc.setElementProperty(
  "caps",
  "video/x-raw,format=RGB,width=320,height=240,bpp=24,depth=24,framerate=0/1"
);

pipeline.play();

setInterval(() => appsrc.push(Buffer.alloc(320 * 240 * 3).fill(randomBytes(3))), 33);
