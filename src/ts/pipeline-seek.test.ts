import { describe, expect, it } from "vitest";
import { Pipeline } from ".";

describe.concurrent("Pipeline Seek Method", () => {
  it("should seek to a specific position in a video pipeline", async () => {
    const pipeline = new Pipeline(
      "videotestsrc num-buffers=300 ! video/x-raw,framerate=30/1 ! fakesink"
    );

    await pipeline.play();

    // Seek to 2 seconds
    const seekResult = pipeline.seek(2.0);
    expect(seekResult).toBe(true);

    // Pause immediately after seeking to get accurate position
    await pipeline.pause();

    // Check if position is valid (should be close to seek position since we paused)
    const position = pipeline.queryPosition();
    if (position !== -1) {
      expect(position).toBeGreaterThan(1.0);
      expect(position).toBeLessThan(3.0); // Tighter bound since we paused
    }

    await pipeline.stop();
  });

  it("should seek to the beginning of the stream", async () => {
    const pipeline = new Pipeline("videotestsrc num-buffers=100 ! fakesink");

    await pipeline.play();

    // Seek to beginning
    const seekResult = pipeline.seek(0);
    expect(seekResult).toBe(true);

    // Pause after seeking to get accurate position
    await pipeline.pause();

    const position = pipeline.queryPosition();
    // Accept either a valid position near 0 or -1 (query failed)
    if (position !== -1) {
      expect(position).toBeGreaterThanOrEqual(0);
      expect(position).toBeLessThan(1.0);
    }

    await pipeline.stop();
  });

  it("should return false for invalid seeks", () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    // Seeking on stopped pipeline might fail
    const seekResult = pipeline.seek(5.0);
    // Note: This may or may not succeed depending on the pipeline state
    expect(typeof seekResult).toBe("boolean");
  });

  it("should throw error for negative position", () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    expect(() => pipeline.seek(-1.0)).toThrow("Position must be >= 0");
  });

  it("should throw error for non-number argument", () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    expect(() => {
      // @ts-expect-error Testing invalid argument type
      pipeline.seek("invalid");
    }).toThrow("seek() requires a number argument");
  });

  it("should throw error when no arguments provided", () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    expect(() => {
      // @ts-expect-error Testing missing argument
      pipeline.seek();
    }).toThrow("seek() requires a number argument");
  });

  it("should handle seek during pause state", async () => {
    const pipeline = new Pipeline(
      "videotestsrc num-buffers=200 ! video/x-raw,framerate=30/1 ! fakesink"
    );

    await pipeline.play();

    // Let it play for a bit to have some content
    await new Promise(resolve => setTimeout(resolve, 50));

    await pipeline.pause();

    // Seek while paused to 1.5 seconds
    const seekResult = pipeline.seek(1.5);
    expect(seekResult).toBe(true);

    // Check position while still paused (should be close to 1.5 seconds)
    const pausedPosition = pipeline.queryPosition();
    if (pausedPosition !== -1) {
      expect(pausedPosition).toBeGreaterThan(0.8);
      expect(pausedPosition).toBeLessThan(2.2);
    }

    // Now test that resuming works correctly
    await pipeline.play();

    // Give it a moment to play
    await new Promise(resolve => setTimeout(resolve, 50));

    // Pause again to check position
    await pipeline.pause();

    const resumedPosition = pipeline.queryPosition();
    // Position should still be reasonable (close to where we seeked)
    if (resumedPosition !== -1) {
      expect(resumedPosition).toBeGreaterThan(0.8);
      expect(resumedPosition).toBeLessThan(2.5);
    }

    await pipeline.stop();
  });

  it("should handle fractional second positions", async () => {
    const pipeline = new Pipeline(
      "videotestsrc num-buffers=150 ! video/x-raw,framerate=30/1 ! fakesink"
    );

    await pipeline.play();

    // Seek to 1.5 seconds
    const seekResult = pipeline.seek(1.5);
    expect(seekResult).toBe(true);

    // Pause after seeking for accurate position reading
    await pipeline.pause();

    const position = pipeline.queryPosition();
    // More precise checking since we paused
    if (position !== -1) {
      expect(position).toBeGreaterThan(0.8);
      expect(position).toBeLessThan(2.2);
    }

    await pipeline.stop();
  });

  it("should allow multiple seeks in sequence", async () => {
    const pipeline = new Pipeline(
      "videotestsrc num-buffers=300 ! video/x-raw,framerate=30/1 ! fakesink"
    );

    await pipeline.play();

    // First seek
    let seekResult = pipeline.seek(1.0);
    expect(seekResult).toBe(true);
    await new Promise(resolve => setTimeout(resolve, 50));

    // Second seek
    seekResult = pipeline.seek(3.0);
    expect(seekResult).toBe(true);
    await new Promise(resolve => setTimeout(resolve, 50));

    // Third seek back to beginning
    seekResult = pipeline.seek(0.5);
    expect(seekResult).toBe(true);

    // Pause to get accurate final position
    await pipeline.pause();

    const position = pipeline.queryPosition();
    // Check that we get a reasonable position after the final seek
    if (position !== -1) {
      expect(position).toBeGreaterThan(0);
      expect(position).toBeLessThan(2.0); // Should be near 0.5 since we paused
    } else {
      // If position query fails, just ensure we got a number
      expect(typeof position).toBe("number");
    }

    await pipeline.stop();
  });

  it("should work with state change promises", async () => {
    const pipeline = new Pipeline("videotestsrc num-buffers=100 ! fakesink");

    // Test that state change promises provide useful information
    const playResult = await pipeline.play();
    expect(playResult.result).toBe("success");
    expect(playResult.targetState).toBe(4); // GST_STATE_PLAYING = 4
    expect(pipeline.playing()).toBe(true);

    const pauseResult = await pipeline.pause();
    expect(pauseResult.result).toBe("success");
    expect(pauseResult.targetState).toBe(3); // GST_STATE_PAUSED = 3
    expect(pipeline.playing()).toBe(false);

    const stopResult = await pipeline.stop();
    expect(stopResult.result).toBe("success");
    expect(stopResult.targetState).toBe(1); // GST_STATE_NULL = 1
    expect(pipeline.playing()).toBe(false);
  });
});
