import { describe, expect, it } from "vitest";
import { Pipeline, type BufferData } from ".";
import { arePluginsAvailable } from "./test-utils";

describe.concurrent("RTP Statistics", () => {
  it.skipIf(!arePluginsAvailable(["x264enc", "rtph264pay", "rtph264depay", "h264parse"]))(
    "should extract RTP stats from rtph264depay stats property",
    async () => {
      const frames = 10;
      const pipeline = new Pipeline(
        `videotestsrc num-buffers=${frames} ! videoconvert ! x264enc ! rtph264pay ! rtph264depay name=depay ! h264parse ! fakesink`
      );

      const depayloader = pipeline.getElementByName("depay");
      if (!depayloader) throw new Error("Expected depayloader element");

      await pipeline.play();

      const statsResult = depayloader.getElementProperty("stats");

      pipeline.stop();

      expect(statsResult).toBeDefined();
      expect(statsResult).not.toBeNull();
      expect(statsResult?.type).toBe("object");

      // Check if stats is an object (GstStructure converted to JS object)
      const stats = statsResult?.value;
      expect(typeof stats).toBe("object");

      if (stats && typeof stats === "object") {
        // Common RTP depayloader stats fields
        const statsObj = stats as any;

        // Stats should have some RTP-related fields
        expect(Object.keys(statsObj).length).toBeGreaterThan(0);

        // Typical stats might include: clock-rate, timestamp, seqnum-base, etc.
        // The exact fields depend on the specific depayloader implementation
      }
    }
  );

  it.skipIf(!arePluginsAvailable(["x264enc", "rtph264pay", "rtph264depay", "h264parse"]))(
    "should compare RTP data from pad probe vs stats property",
    async () => {
      const frames = 5;
      const pipeline = new Pipeline(
        `videotestsrc num-buffers=${frames} ! videoconvert ! x264enc ! rtph264pay name=pay ! rtph264depay name=depay ! h264parse ! fakesink`
      );

      const payloader = pipeline.getElementByName("pay");
      const depayloader = pipeline.getElementByName("depay");

      if (!payloader) throw new Error("Expected payloader element");
      if (!depayloader) throw new Error("Expected depayloader element");

      let padProbeRtpData: any = null;

      // Set up pad probe on payloader to capture RTP data
      const [bufferData, unsubscribe]: [BufferData, () => void] = await new Promise(resolve => {
        const unsubscribe = payloader.addPadProbe("src", (bufferData: BufferData) => {
          if (bufferData.rtp) {
            resolve([bufferData, unsubscribe]);
          }
        });

        pipeline.play();
      });

      padProbeRtpData = bufferData.rtp;
      unsubscribe();

      // Wait a bit more for stats to accumulate
      await new Promise(resolve => setTimeout(resolve, 50));

      const statsResult = depayloader.getElementProperty("stats");

      pipeline.stop();

      // Verify pad probe captured RTP data
      expect(padProbeRtpData).toBeDefined();
      expect(typeof padProbeRtpData.timestamp).toBe("number");
      expect(typeof padProbeRtpData.sequence).toBe("number");

      // Verify stats property returns data
      expect(statsResult).toBeDefined();
      expect(statsResult).not.toBeNull();
      expect(statsResult?.type).toBe("object");
    }
  );

  it("should handle different types of stats properties", async () => {
    // Test with an element that has different stats (not RTP)
    const pipeline = new Pipeline("videotestsrc ! fakesink name=sink");

    const sink = pipeline.getElementByName("sink");
    if (!sink) throw new Error("Expected sink element");

    await pipeline.play();

    const statsResult = sink.getElementProperty("stats");

    pipeline.stop();

    // fakesink has stats but different type (basesink stats, not RTP depayload stats)
    expect(statsResult).toBeDefined();
    expect(statsResult).not.toBeNull();
    expect(statsResult?.type).toBe("object");

    const stats = statsResult?.value;
    if (stats && typeof stats === "object") {
      const statsObj = stats as any;
      expect(statsObj.name).not.toBe("application/x-rtp-depayload-stats");
    }
  });
});
