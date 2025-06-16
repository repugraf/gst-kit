import { describe, expect, it } from "vitest";
import { Pipeline, type GStreamerSample } from ".";

describe("FakeSink", () => {
  it("should capture last sample when enabled", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink enable-last-sample=true name=sink");
    const fakesink = pipeline.getElementByName("sink");

    if (!fakesink) throw new Error("FakeSink element not found");

    pipeline.play();

    // Wait for some frames to be processed
    await new Promise(resolve => setTimeout(resolve, 50));

    // Get the last sample
    const sample = fakesink.getElementProperty("last-sample") as GStreamerSample;

    pipeline.stop();

    expect(sample).not.toBeNull();
    expect(sample).toHaveProperty("buffer");
    expect(sample).toHaveProperty("caps");
    expect(sample).toHaveProperty("flags");

    if (sample && typeof sample === "object" && "buf" in sample && "caps" in sample) {
      expect(sample.buffer).toBeInstanceOf(Buffer);
      expect(sample.buffer?.length).toBeGreaterThan(0);
      expect(sample.caps).toHaveProperty("name");
      expect(typeof sample.flags).toBe("number");
    }
  });

  it("should return null when last-sample is disabled", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink enable-last-sample=false name=sink");
    const fakesink = pipeline.getElementByName("sink");

    if (!fakesink) throw new Error("FakeSink element not found");

    pipeline.play();

    // Wait for some frames to be processed
    await new Promise(resolve => setTimeout(resolve, 50));

    // Try to get the last sample (should be null since it's disabled)
    const sample = fakesink.getElementProperty("last-sample");

    pipeline.stop();

    expect(sample).toBeNull();
  });

  it("should handle pipeline state changes correctly", () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink enable-last-sample=true name=sink");
    const fakesink = pipeline.getElementByName("sink");

    if (!fakesink) throw new Error("FakeSink element not found");

    // Pipeline should not be playing initially
    expect(pipeline.playing()).toBe(false);

    pipeline.play();
    // Note: We don't check playing() immediately after play() since state transitions are async

    pipeline.stop();
    expect(pipeline.playing()).toBe(false);
  });

  it("should work with limited number of buffers", async () => {
    const numBuffers = 5;
    const pipeline = new Pipeline(
      `videotestsrc num-buffers=${numBuffers} ! fakesink enable-last-sample=true name=sink`
    );
    const fakesink = pipeline.getElementByName("sink");

    if (!fakesink) throw new Error("FakeSink element not found");

    pipeline.play();

    // Wait for all buffers to be processed
    await new Promise(resolve => setTimeout(resolve, 50));

    const sample = fakesink.getElementProperty("last-sample");

    pipeline.stop();

    expect(sample).not.toBeNull();
    if (sample && typeof sample === "object" && "buf" in sample) {
      expect((sample as any).buf).toBeInstanceOf(Buffer);
      expect((sample as any).buf.length).toBeGreaterThan(0);
    }
  });

  it("should capture samples with different video formats", async () => {
    const pipeline = new Pipeline(
      "videotestsrc ! videoconvert ! video/x-raw,format=RGB ! fakesink enable-last-sample=true name=sink"
    );
    const fakesink = pipeline.getElementByName("sink");

    if (!fakesink) throw new Error("FakeSink element not found");

    pipeline.play();

    await new Promise(resolve => setTimeout(resolve, 50));

    const sample = fakesink.getElementProperty("last-sample") as GStreamerSample;

    pipeline.stop();

    expect(sample).not.toBeNull();
    if (sample && typeof sample === "object" && "caps" in sample) {
      const caps = sample.caps;
      expect(caps.name).toContain("video/x-raw");
    }
  });
});
