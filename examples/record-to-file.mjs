#!/usr/bin/env node
/**
 * Stream Recording Example with EOS (End of Stream) Handling
 *
 * Records programmatically generated video frames to an OGV file.
 * Creates colorful animated frames and properly handles EOS.
 */
import { Pipeline } from "../dist/esm/index.mjs";
import path from "node:path";
import fs from "node:fs";
import { randomBytes } from "node:crypto";

// Output file path
const outputFile = path.join(process.cwd(), "recorded_stream.ogv");

// Remove existing file if it exists
if (fs.existsSync(outputFile)) {
  fs.unlinkSync(outputFile);
  console.log("🗑️  Removed existing output file");
}

// Create pipeline
const pipeline = new Pipeline(`
  appsrc name=mysource ! 
  videoconvert ! 
  theoraenc ! 
  oggmux ! 
  filesink location=${outputFile}
`);

const appsrc = pipeline.getElementByName("mysource");

// Set video caps and properties
appsrc.setElementProperty("caps", "video/x-raw,format=RGB,width=640,height=480,framerate=30/1");
appsrc.setElementProperty("format", "time");
appsrc.setElementProperty("is-live", false);
appsrc.setElementProperty("do-timestamp", true);

console.log(`🎬 Starting recording to: ${outputFile}`);
pipeline.play();

let frameCount = 0;

// Generate frames with different colors
const pushInterval = setInterval(() => {
  frameCount++;
  appsrc.push(Buffer.alloc(640 * 480 * 3).fill(randomBytes(3)));

  if (frameCount % 30 === 0) {
    console.log(`📹 Recorded ${frameCount} frames (${frameCount / 30} seconds)`);
  }
}, 1000 / 30); // 30 FPS

console.log("🛑 Will send End-of-Stream signal in 10 seconds...");
setTimeout(() => {
  console.log("🏁 Sending End-of-Stream now!");
  clearInterval(pushInterval);
  setTimeout(() => appsrc.endOfStream(), 100);
}, 10000);

// Message loop to handle EOS
while (true) {
  const message = await pipeline.busPop(100);
  if (!message) continue;

  if (message.type === "qos") continue; // Filter noisy messages

  console.log(`📨 Bus message: ${message.type}`);

  if (message.type === "eos") {
    console.log("🎉 End-of-Stream received! Finalizing recording...");
    pipeline.stop();
    break;
  } else if (message.type === "error") {
    console.error("❌ Pipeline error:", message.message || "Unknown error");
    pipeline.stop();
    clearInterval(pushInterval);
    break;
  }
}

if (fs.existsSync(outputFile)) {
  const stats = fs.statSync(outputFile);
  console.log(`✅ Recording complete! File size: ${(stats.size / 1024 / 1024).toFixed(2)} MB`);
  console.log(`📁 Output file: ${outputFile}`);
} else {
  console.log("❌ Output file was not created");
}
