import { describe, expect, it } from "vitest";
import { Pipeline, type GstMessage } from ".";

describe.concurrent("AppSrc End-of-Stream", () => {
  it("should send EOS signal through endOfStream method", async () => {
    const pipeline = new Pipeline("appsrc name=source ! fakesink");
    const source = pipeline.getElementByName("source");

    expect(source?.type).toBe("app-src-element");

    if (source?.type === "app-src-element") {
      // Set up caps for the source
      source.setElementProperty(
        "caps",
        "video/x-raw,format=RGB,width=320,height=240,framerate=30/1"
      );

      await pipeline.play();

      // Push a few buffers
      const buffer = Buffer.alloc(320 * 240 * 3);
      buffer.fill(0xff); // Fill with white pixels

      for (let i = 0; i < 3; i++) {
        source.push(buffer);
      }

      // Send end-of-stream
      source.endOfStream();

      // Wait for EOS message on the bus
      let eosReceived = false;
      let attempts = 0;
      const maxAttempts = 20; // Increased attempts

      while (!eosReceived && attempts < maxAttempts) {
        const message: GstMessage | null = await pipeline.busPop(1000); // Increased timeout
        attempts++;

        if (message?.type === "eos") {
          eosReceived = true;
          expect(message.type).toBe("eos");
          break;
        }
      }

      await pipeline.stop();

      expect(eosReceived).toBe(true);
    }
  });

  it("should reject endOfStream on non-AppSrc elements", async () => {
    const pipeline = new Pipeline("videotestsrc name=source ! fakesink");
    const source = pipeline.getElementByName("source");

    expect(source?.type).toBe("element");

    if (source?.type === "element") {
      // Attempting to call endOfStream on a non-AppSrc element should throw
      expect(() => {
        // @ts-expect-error - Testing runtime error for non-AppSrc element
        source.endOfStream();
      }).toThrow();
    }

    await pipeline.stop();
  });

  it("should handle multiple endOfStream calls gracefully", async () => {
    const pipeline = new Pipeline("appsrc name=source ! fakesink");
    const source = pipeline.getElementByName("source");

    if (source?.type === "app-src-element") {
      source.setElementProperty(
        "caps",
        "video/x-raw,format=RGB,width=320,height=240,framerate=30/1"
      );

      await pipeline.play();

      // Send first EOS
      source.endOfStream();

      // Try to send second EOS - should handle gracefully
      try {
        source.endOfStream();
        // Second call might succeed or fail depending on GStreamer state,
        // but it shouldn't crash the application
      } catch (error) {
        // It's acceptable for the second call to fail
        expect(error).toBeDefined();
      }

      await pipeline.stop();
    }
  });
});
