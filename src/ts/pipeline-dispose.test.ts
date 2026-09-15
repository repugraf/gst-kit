import { describe, expect, it } from "vitest";
import { Pipeline } from ".";

describe("Pipeline dispose()", () => {
  it("should dispose a pipeline that was never played", () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    // Disposing a freshly constructed (never played) pipeline is valid and
    // should not throw.
    expect(() => pipeline.dispose()).not.toThrow();
  });

  it("should dispose a pipeline after play and stop", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    await pipeline.play();
    expect(pipeline.playing()).toBe(true);
    await pipeline.stop();

    expect(() => pipeline.dispose()).not.toThrow();
  });

  it("should dispose a still-playing pipeline by forcing it to NULL first", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    await pipeline.play();
    expect(pipeline.playing()).toBe(true);

    // Callers are expected to stop() first, but dispose() must not leak if they
    // don't: rather than throw (which would skip the unref and leak the native
    // pipeline), dispose() drives the pipeline to NULL synchronously and then
    // releases it. So disposing a still-playing pipeline is safe and does not
    // throw.
    expect(() => pipeline.dispose()).not.toThrow();

    // After dispose() the pipeline is released; any further use throws.
    expect(() => pipeline.playing()).toThrow(/used after dispose/);
  });

  it("should be safe to dispose after a worker started against the pipeline resolves", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    await pipeline.play();

    // Start an async worker (busPop) that holds its own gst_object_ref while it
    // runs. The pipeline is stopped and the worker awaited before dispose(), so
    // the reference the worker held is already released.
    const pending = pipeline.busPop(1000);
    await pipeline.stop();
    await pending;

    expect(() => pipeline.dispose()).not.toThrow();
  });

  it("should dispose safely while a state-change worker is still in flight", async () => {
    const pipeline = new Pipeline("videotestsrc ! videoconvert ! queue ! fakesink");

    // Start play() but do NOT await it: the StateChangeWorker is queued and will
    // run on a background thread after dispose() returns. dispose() must defer
    // its native teardown until that worker finishes, otherwise the worker
    // drives the state back up and finalizes a non-NULL pipeline (a leak).
    const playing = pipeline.play();

    // dispose() while the worker is in flight — marks disposed immediately and
    // defers the release.
    expect(() => pipeline.dispose()).not.toThrow();

    // The pipeline is observably disposed right away.
    expect(() => pipeline.playing()).toThrow(/used after dispose/);

    // Awaiting the in-flight worker completes without error; the deferred
    // teardown runs when it finishes.
    await expect(playing).resolves.toBeDefined();

    // Still disposed, and a second dispose() is a no-op.
    expect(() => pipeline.dispose()).not.toThrow();
  });

  it("should not grow RSS when disposing with a state change in flight repeatedly", async () => {
    // Guards finding 1: a queued state-change worker must not finalize a
    // non-NULL pipeline after dispose(). Without the deferred teardown this loop
    // leaks several MB per iteration; with it, RSS stays essentially flat.
    const iterations = 100;

    const build = () => new Pipeline("videotestsrc ! videoconvert ! queue ! fakesink");

    // Warm up so first-touch allocations don't skew the baseline.
    for (let i = 0; i < 10; i++) {
      const p = build();
      const playing = p.play();
      p.dispose();
      await playing;
    }
    if (typeof globalThis.gc === "function") globalThis.gc();
    const before = process.memoryUsage().rss;

    for (let i = 0; i < iterations; i++) {
      const p = build();
      const playing = p.play(); // in flight
      p.dispose(); // deferred teardown
      await playing; // teardown runs here
    }

    if (typeof globalThis.gc === "function") globalThis.gc();
    const after = process.memoryUsage().rss;

    const growthMb = (after - before) / 1024 / 1024;
    // A per-iteration leak of the ~3 MB pipeline would be hundreds of MB over
    // 100 iterations. Allow generous headroom for allocator/GC noise.
    expect(growthMb).toBeLessThan(50);
  });

  it("should be idempotent — a second dispose() is a no-op", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    await pipeline.play();
    await pipeline.stop();

    pipeline.dispose();
    // Calling dispose() again must not throw.
    expect(() => pipeline.dispose()).not.toThrow();
  });

  it("should throw on synchronous method calls after dispose()", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    await pipeline.play();
    await pipeline.stop();
    pipeline.dispose();

    // Every driver-delegating method routes through the native disposed guard,
    // so a use-after-dispose is a loud error rather than a native null-deref.
    expect(() => pipeline.playing()).toThrow(/used after dispose/);
    expect(() => pipeline.queryPosition()).toThrow(/used after dispose/);
    expect(() => pipeline.queryDuration()).toThrow(/used after dispose/);
    expect(() => pipeline.getElementByName("sink")).toThrow(/used after dispose/);
    expect(() => pipeline.seek(0)).toThrow(/used after dispose/);
    expect(() => pipeline.endOfStream()).toThrow(/used after dispose/);
  });

  it("should throw on async method calls after dispose()", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    await pipeline.play();
    await pipeline.stop();
    pipeline.dispose();

    // Async methods throw synchronously (at call time) because the disposed
    // guard runs before the worker is queued.
    expect(() => pipeline.play()).toThrow(/used after dispose/);
    expect(() => pipeline.pause()).toThrow(/used after dispose/);
    expect(() => pipeline.stop()).toThrow(/used after dispose/);
    expect(() => pipeline.busPop(0)).toThrow(/used after dispose/);
  });
});
