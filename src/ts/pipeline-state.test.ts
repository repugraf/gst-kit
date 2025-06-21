import { describe, expect, it } from "vitest";
import { Pipeline } from ".";

describe("Pipeline State Management", () => {
  it("should play a pipeline", () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    pipeline.play();

    // Check that the pipeline is playing
    expect(pipeline.playing()).toBe(true);

    pipeline.stop();
  });

  it("should pause a pipeline", () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    pipeline.play();
    expect(pipeline.playing()).toBe(true);

    pipeline.pause();

    // After pausing, pipeline should not be in playing state
    expect(pipeline.playing()).toBe(false);

    pipeline.stop();
  });

  it("should stop a pipeline", () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    pipeline.play();
    expect(pipeline.playing()).toBe(true);

    pipeline.stop();

    // After stopping, pipeline should not be playing
    expect(pipeline.playing()).toBe(false);
  });

  it("should handle play -> pause -> play transitions", () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    // Start playing
    pipeline.play();
    expect(pipeline.playing()).toBe(true);

    // Pause
    pipeline.pause();
    expect(pipeline.playing()).toBe(false);

    // Resume playing
    pipeline.play();
    expect(pipeline.playing()).toBe(true);

    pipeline.stop();
  });

  it("should handle pause from stopped state", () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    // Pipeline starts in stopped state
    expect(pipeline.playing()).toBe(false);

    // Pause from stopped state should work
    pipeline.pause();
    expect(pipeline.playing()).toBe(false);

    pipeline.stop();
  });

  it("should handle multiple state transitions", () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    // Test various state transitions
    pipeline.play();
    expect(pipeline.playing()).toBe(true);

    pipeline.pause();
    expect(pipeline.playing()).toBe(false);

    pipeline.pause(); // pause again (should be safe)
    expect(pipeline.playing()).toBe(false);

    pipeline.play();
    expect(pipeline.playing()).toBe(true);

    pipeline.play(); // play again (should be safe)
    expect(pipeline.playing()).toBe(true);

    pipeline.stop();
    expect(pipeline.playing()).toBe(false);

    pipeline.stop(); // stop again (should be safe)
    expect(pipeline.playing()).toBe(false);
  });

  it("should maintain position information across pause/resume", async () => {
    const pipeline = new Pipeline("videotestsrc num-buffers=100 ! fakesink");

    pipeline.play();

    const positionBeforePause = pipeline.queryPosition();

    pipeline.pause();

    // Wait while paused
    await new Promise(resolve => setTimeout(resolve, 50));

    const positionWhilePaused = pipeline.queryPosition();

    pipeline.play();

    // Wait a bit more
    await new Promise(resolve => setTimeout(resolve, 50));

    const positionAfterResume = pipeline.queryPosition();

    pipeline.stop();

    // Position during pause should be maintained or at least not decrease significantly
    expect(positionWhilePaused).toBeGreaterThanOrEqual(positionBeforePause - 0.1);

    // Position after resume should be greater than or equal to pause position
    expect(positionAfterResume).toBeGreaterThanOrEqual(positionWhilePaused);
  });
});
