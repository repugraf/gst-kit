import { describe, expect, it } from "vitest";
import { Pipeline, type GstMessage } from ".";

describe.concurrent("Pipeline EOS - State-Gated Dispatch", () => {
  it("should return true when pipeline is in PLAYING state", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    await pipeline.play();

    const result = pipeline.endOfStream();
    expect(result).toBe(true);

    await pipeline.stop();
  });

  it("should return true when pipeline is in PAUSED state", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink async=false");

    await pipeline.pause();

    const result = pipeline.endOfStream();
    expect(result).toBe(true);

    await pipeline.stop();
  });

  it("should return false when pipeline is in NULL state (freshly constructed)", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    const result = pipeline.endOfStream();
    expect(result).toBe(false);

    await pipeline.stop();
  });

  it("should return a boolean synchronously (not a Promise)", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    await pipeline.play();

    const result = pipeline.endOfStream();

    // Verify it's a boolean type
    expect(typeof result).toBe("boolean");

    // Verify it's not a Promise (confirming synchronous behavior)
    expect(result).not.toBeInstanceOf(Promise);

    await pipeline.stop();
  });

  it("should propagate EOS message on the bus after endOfStream", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    await pipeline.play();

    const sent = pipeline.endOfStream();
    expect(sent).toBe(true);

    // Poll busPop for an EOS message within a reasonable timeout
    let eosMessage: GstMessage | null = null;
    const maxAttempts = 20;

    for (let i = 0; i < maxAttempts; i++) {
      const message = await pipeline.busPop(500);
      if (message && message.type === "eos") {
        eosMessage = message;
        break;
      }
    }

    await pipeline.stop();

    expect(eosMessage).not.toBeNull();
    expect(eosMessage!.type).toBe("eos");
  });

  it("should handle multiple endOfStream calls on a PLAYING pipeline without exception", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    await pipeline.play();

    const result1 = pipeline.endOfStream();
    const result2 = pipeline.endOfStream();
    const result3 = pipeline.endOfStream();

    expect(result1).toBe(true);
    expect(result2).toBe(true);
    expect(result3).toBe(true);

    await pipeline.stop();
  });

  it("should return false when endOfStream is called after stopping the pipeline", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    await pipeline.play();

    const resultWhilePlaying = pipeline.endOfStream();
    expect(resultWhilePlaying).toBe(true);

    await pipeline.stop();

    const resultAfterStop = pipeline.endOfStream();
    expect(resultAfterStop).toBe(false);
  });
});
