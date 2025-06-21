import { describe, expect, it } from "vitest";
import { Pipeline } from ".";

describe.concurrent("Pipeline State Management", () => {
  it("should play a pipeline", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    await pipeline.play();

    // Check that the pipeline is playing
    expect(pipeline.playing()).toBe(true);

    await pipeline.stop();
  });

  it("should pause a pipeline", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    await pipeline.play();
    expect(pipeline.playing()).toBe(true);

    await pipeline.pause();

    // After pausing, pipeline should not be in playing state
    expect(pipeline.playing()).toBe(false);

    await pipeline.stop();
  });

  it("should stop a pipeline", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    await pipeline.play();
    expect(pipeline.playing()).toBe(true);

    await pipeline.stop();

    // After stopping, pipeline should not be playing
    expect(pipeline.playing()).toBe(false);
  });

  it("should handle play -> pause -> play transitions", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    // Start playing
    await pipeline.play();
    expect(pipeline.playing()).toBe(true);

    // Pause
    await pipeline.pause();
    expect(pipeline.playing()).toBe(false);

    // Resume playing
    await pipeline.play();
    expect(pipeline.playing()).toBe(true);

    await pipeline.stop();
  });

  it("should handle pause from stopped state", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    // Pipeline starts in stopped state
    expect(pipeline.playing()).toBe(false);

    // Pause from stopped state should work
    await pipeline.pause();
    expect(pipeline.playing()).toBe(false);

    await pipeline.stop();
  });

  it("should handle multiple state transitions", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    // Test various state transitions
    await pipeline.play();
    expect(pipeline.playing()).toBe(true);

    await pipeline.pause();
    expect(pipeline.playing()).toBe(false);

    await pipeline.pause(); // pause again (should be safe)
    expect(pipeline.playing()).toBe(false);

    await pipeline.play();
    expect(pipeline.playing()).toBe(true);

    await pipeline.play(); // play again (should be safe)
    expect(pipeline.playing()).toBe(true);

    await pipeline.stop();
    expect(pipeline.playing()).toBe(false);

    await pipeline.stop(); // stop again (should be safe)
    expect(pipeline.playing()).toBe(false);
  });

  it("should handle position queries during state transitions", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    await pipeline.play();

    // Let it play for a bit
    await new Promise(resolve => setTimeout(resolve, 100));

    const positionWhilePlaying = pipeline.queryPosition();
    expect(positionWhilePlaying).toBeGreaterThan(0); // Should have some position

    await pipeline.pause();

    // Position queries should still work when paused (position may vary)
    const positionWhilePaused = pipeline.queryPosition();
    expect(positionWhilePaused).toBeGreaterThanOrEqual(0); // Should be valid

    await pipeline.play();

    // Let it continue playing
    await new Promise(resolve => setTimeout(resolve, 100));

    const positionAfterResume = pipeline.queryPosition();
    expect(positionAfterResume).toBeGreaterThan(0); // Should still have position

    await pipeline.stop();

    // Position tracking should be functional throughout the test
    expect(positionWhilePlaying).toBeGreaterThan(0);
    expect(positionWhilePaused).toBeGreaterThanOrEqual(0);
    expect(positionAfterResume).toBeGreaterThan(0);
  });

  it("should provide detailed state change information", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    // Test play state change
    const playResult = await pipeline.play();
    expect(playResult.result).toBe("success");
    expect(playResult.targetState).toBe(4); // GST_STATE_PLAYING
    expect(playResult.finalState).toBe(4);

    // Test pause state change
    const pauseResult = await pipeline.pause();
    expect(pauseResult.result).toBe("success");
    expect(pauseResult.targetState).toBe(3); // GST_STATE_PAUSED
    expect(pauseResult.finalState).toBe(3);

    // Test stop state change
    const stopResult = await pipeline.stop();
    expect(stopResult.result).toBe("success");
    expect(stopResult.targetState).toBe(1); // GST_STATE_NULL
    expect(stopResult.finalState).toBe(1);
  });

  it("should accept timeout in milliseconds", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    // Test with custom timeout in milliseconds
    const playResult = await pipeline.play(2000); // 2 seconds in milliseconds
    expect(playResult.result).toBe("success");
    expect(playResult.targetState).toBe(4); // GST_STATE_PLAYING
    expect(playResult.finalState).toBe(4);

    const pauseResult = await pipeline.pause(1500); // 1.5 seconds in milliseconds
    expect(pauseResult.result).toBe("success");
    expect(pauseResult.targetState).toBe(3); // GST_STATE_PAUSED
    expect(pauseResult.finalState).toBe(3);

    const stopResult = await pipeline.stop(500); // 0.5 seconds in milliseconds
    expect(stopResult.result).toBe("success");
    expect(stopResult.targetState).toBe(1); // GST_STATE_NULL
    expect(stopResult.finalState).toBe(1);
  });
});
