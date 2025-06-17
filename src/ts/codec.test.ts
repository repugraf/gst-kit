import { describe, expect, it } from "vitest";
import { Pipeline, type GStreamerSample, GstBufferFlags, type BufferData } from ".";

describe("codec", () => {
  it("should be able to distinguish between delta and key frames", async () => {
    const frames = 5;
    const pipeline = new Pipeline(
      `videotestsrc num-buffers=${frames} ! videoconvert ! x264enc ! rtph264pay ! rtph264depay ! h264parse ! appsink name=sink`
    );
    const sink = pipeline.getElementByName("sink");

    if (sink?.type !== "app-sink-element") throw new Error("Expected app sink element");

    pipeline.play();

    const samples: GStreamerSample[] = [];

    let currFrames = 0;
    while (true) {
      const result = await sink.getSample();
      if (!result) break;
      samples.push(result);
      currFrames++;
    }

    pipeline.stop();

    const keyFrame = samples.find(e =>
      e.flags ? !(e.flags & GstBufferFlags.GST_BUFFER_FLAG_DELTA_UNIT) : false
    );
    const deltaFrame = samples.find(e =>
      e.flags ? !!(e.flags & GstBufferFlags.GST_BUFFER_FLAG_DELTA_UNIT) : false
    );

    expect(keyFrame).toBeDefined();
    expect(deltaFrame).toBeDefined();
  });

  it("should be able to extract rtp timestamp using pad probe", async () => {
    const frames = 5;
    const pipeline = new Pipeline(
      `videotestsrc num-buffers=${frames} ! videoconvert ! x264enc ! rtph264pay name=pay ! fakesink`
    );
    const payloader = pipeline.getElementByName("pay");

    if (!payloader) throw new Error("Expected payloader element");

    const [bufferData, unsubscribe]: [BufferData, () => void] = await new Promise(resolve => {
      const unsubscribe = payloader.addPadProbe("src", (bufferData: BufferData) => {
        resolve([bufferData, unsubscribe]);
      });

      pipeline.play();
    });

    unsubscribe();
    pipeline.stop();

    expect(bufferData).toBeDefined();
    expect(bufferData.buffer).toBeDefined();
    expect(bufferData.buffer!.length).toBeGreaterThan(0);
    expect(bufferData.flags).toBeDefined();

    // Check RTP-specific data
    expect(bufferData.rtp).toBeDefined();
    expect(typeof bufferData.rtp!.timestamp).toBe("number");
    expect(typeof bufferData.rtp!.sequence).toBe("number");
    expect(typeof bufferData.rtp!.ssrc).toBe("number");
    expect(typeof bufferData.rtp!.payloadType).toBe("number");

    // H.264 payload type is typically 96-127 for dynamic payloads
    expect(bufferData.rtp!.payloadType).toBeGreaterThanOrEqual(96);
  });
});
