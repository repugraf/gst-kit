import { Pipeline } from "../dist/esm/index.mjs";

console.log("Checking GStreamer Element Availability\n");

// Check core elements
console.log("Core Elements:");
console.log(`  videotestsrc: ${Pipeline.elementExists("videotestsrc")}`);
console.log(`  audiotestsrc: ${Pipeline.elementExists("audiotestsrc")}`);
console.log(`  fakesink: ${Pipeline.elementExists("fakesink")}`);
console.log(`  appsrc: ${Pipeline.elementExists("appsrc")}`);
console.log(`  appsink: ${Pipeline.elementExists("appsink")}`);

// Check encoders
console.log("\nVideo Encoders:");
console.log(`  x264enc (H.264): ${Pipeline.elementExists("x264enc")}`);
console.log(`  x265enc (H.265): ${Pipeline.elementExists("x265enc")}`);
console.log(`  vp8enc (VP8): ${Pipeline.elementExists("vp8enc")}`);
console.log(`  vp9enc (VP9): ${Pipeline.elementExists("vp9enc")}`);

// Check hardware acceleration
console.log("\nHardware Acceleration:");
console.log(`  vaapih264enc (Intel VAAPI): ${Pipeline.elementExists("vaapih264enc")}`);
console.log(`  nvh264enc (NVIDIA NVENC): ${Pipeline.elementExists("nvh264enc")}`);

// Check non-existent elements
console.log("\nNon-existent Elements:");
console.log(`  fakeelement: ${Pipeline.elementExists("fakeelement")}`);
console.log(`  notreal123: ${Pipeline.elementExists("notreal123")}`);

// Practical use case: Choose encoder based on availability
console.log("\nPractical Example - Choosing Best Available Encoder:");
if (Pipeline.elementExists("nvh264enc")) {
  console.log("  Using NVIDIA hardware encoder");
} else if (Pipeline.elementExists("vaapih264enc")) {
  console.log("  Using Intel VAAPI hardware encoder");
} else if (Pipeline.elementExists("x264enc")) {
  console.log("  Using x264 software encoder");
} else {
  console.log("  No H.264 encoder available!");
}
