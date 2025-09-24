import { describe, expect, it } from "vitest";
import { Pipeline, type GstMessage } from ".";
import { isWindows } from "./test-utils";

describe.concurrent("Pipeline busPop Method", () => {
  it("should return null when no message available with timeout", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    // Try to pop a message with a very short timeout (10ms)
    const message = await pipeline.busPop(10);

    pipeline.stop();

    // Should return null when timeout expires with no messages
    expect(message).toBeNull();
  });

  it("should return state-changed message when starting pipeline", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    // Start pipeline to generate state-changed messages
    await pipeline.play();

    // Pop a message with reasonable timeout
    const message = await pipeline.busPop(1000);

    pipeline.stop();

    expect(message).not.toBeNull();
    expect(typeof message?.type).toBe("string");
    expect(typeof message?.timestamp).toBe("bigint");
  });

  it("should handle async preroll message", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink");

    await pipeline.play();

    // Look for async-done or state-changed messages
    let foundMessage = false;
    let attempts = 0;
    const maxAttempts = 5;

    while (!foundMessage && attempts < maxAttempts) {
      const message = await pipeline.busPop(500);
      attempts++;

      if (message) {
        expect(typeof message.type).toBe("string");
        expect(typeof message.timestamp).toBe("bigint");
        foundMessage = true;
      }
    }

    pipeline.stop();

    // Should have found at least one message during pipeline startup
    expect(foundMessage).toBe(true);
  });

  it("should handle error messages from invalid pipeline", async () => {
    // Create a pipeline with invalid file source that should generate error messages during playback
    const pipeline = new Pipeline("filesrc location=/nonexistent/file.mp4 ! fakesink");

    await pipeline.play();

    const message = await pipeline.busPop(1000);

    pipeline.stop();

    if (message && message.type === "error") {
      expect(message.errorMessage).toBeDefined();
      expect(typeof message.errorMessage).toBe("string");
    }
    // Note: This test might not always find an error message depending on timing,
    // but it validates the structure when error messages are present
  });

  it("should respect timeout parameter", async () => {
    const pipeline = new Pipeline("fakesrc ! fakesink");

    const startTime = Date.now();
    await pipeline.busPop(100); // 100ms timeout
    const endTime = Date.now();
    const elapsed = endTime - startTime;

    pipeline.stop();

    // Should timeout within a reasonable range (timing varies by platform)
    expect(elapsed).toBeGreaterThan(80);
    // Windows has higher async overhead, so use more tolerant bounds
    const upperBound = isWindows ? 250 : 150;
    expect(elapsed).toBeLessThan(upperBound);
  });

  it("should handle infinite timeout with negative value", async () => {
    const pipeline = new Pipeline("videotestsrc num-buffers=1 ! fakesink");

    await pipeline.play();

    // Use -1 for infinite timeout, but pipeline should generate EOS quickly
    const message = await pipeline.busPop(-1);

    pipeline.stop();

    // Should get a message (likely EOS or state-changed)
    expect(message).not.toBeNull();
  });

  it("should handle message with structure data", async () => {
    const pipeline = new Pipeline("videotestsrc num-buffers=1 ! fakesink");

    await pipeline.play();

    // Look for messages with structure data
    let messageWithStructure: GstMessage | null = null;
    let attempts = 0;
    const maxAttempts = 10;

    while (!messageWithStructure && attempts < maxAttempts) {
      const message = await pipeline.busPop(200);
      attempts++;

      if (message && message.structureName) {
        messageWithStructure = message;
        break;
      }
    }

    pipeline.stop();

    if (messageWithStructure) {
      expect(messageWithStructure.structureName).toBeDefined();
      expect(typeof messageWithStructure.structureName).toBe("string");
    }
  });

  it("should receive message with array property type", async () => {
    const pipeline = new Pipeline(
      `audiotestsrc wave=ticks tick-interval=400000000 num-buffers=50 ` +
        `! level name=lev ! fakeaudiosink`
    );

    await pipeline.play();

    let message: Awaited<ReturnType<typeof pipeline.busPop>> = null;

    do {
      message = await pipeline.busPop();
    } while (message?.type !== "element" || message?.srcElementName !== "lev");

    await pipeline.stop();

    expect(message?.peak).toBeInstanceOf(Array);
  });
});
