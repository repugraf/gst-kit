import { describe, expect, it } from "vitest";
import { Pipeline } from ".";

describe.concurrent("Pipeline Query Methods", () => {
  it("should query position from videotestsrc pipeline", async () => {
    const pipeline = new Pipeline("videotestsrc num-buffers=100 ! fakesink");

    await pipeline.play();

    const position = pipeline.queryPosition();

    pipeline.stop();

    // Position should be a number (could be -1 if not available)
    expect(typeof position).toBe("number");
  });

  it("should query duration from videotestsrc pipeline", async () => {
    const pipeline = new Pipeline("videotestsrc num-buffers=100 ! fakesink");

    await pipeline.play();

    const duration = pipeline.queryDuration();

    pipeline.stop();

    // Duration should be a number (could be -1 if not available)
    expect(typeof duration).toBe("number");
  });

  it("should return -1 for position when pipeline is not playing", () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    expect(pipeline.queryPosition()).toBe(-1);
  });

  it("should return -1 for duration when pipeline is not playing", () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    // Should return -1 when pipeline is not playing
    expect(pipeline.queryDuration()).toBe(-1);
  });

  it("should query position and duration during playback", async () => {
    const pipeline = new Pipeline("videotestsrc num-buffers=50 ! fakesink");

    await pipeline.play();

    const position1 = pipeline.queryPosition();
    const duration = pipeline.queryDuration();

    // Wait a bit
    await new Promise(resolve => setTimeout(resolve, 50));

    const position2 = pipeline.queryPosition();

    pipeline.stop();

    expect(typeof position1).toBe("number");
    expect(typeof position2).toBe("number");
    expect(typeof duration).toBe("number");

    // Position should advance over time (or remain the same if very short)
    expect(position2).toBeGreaterThanOrEqual(position1);
  });
});
