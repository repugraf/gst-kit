import { describe, expect, it } from "vitest";
import { Pipeline, type GStreamerSample } from ".";
import { arePluginsAvailable } from "./test-utils";

describe.concurrent("AppSink", () => {
  it("should pull frames", async () => {
    const pipeline = new Pipeline("videotestsrc ! videoconvert ! appsink name=sink");
    const sink = pipeline.getElementByName("sink");

    if (sink?.type !== "app-sink-element") throw new Error("Expected app sink element");

    await pipeline.play();

    const result = await sink.getSample();

    pipeline.stop();

    expect(result).not.toBeNull();
    expect(result?.buffer).toBeDefined();
  });

  it.skipIf(!arePluginsAvailable(["rtph264depay", "h264parse", "avdec_h264"]))(
    "should return null on pull if no frames received",
    async () => {
      const pipeline = new Pipeline(
        "udpsrc address=127.0.0.1 port=5004 ! queue " +
          "! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! appsink name=sink"
      );
      const sink = pipeline.getElementByName("sink");

      if (sink?.type !== "app-sink-element") throw new Error("Expected app sink element");

      await pipeline.play(10);

      const result = await sink.getSample(10);

      pipeline.stop();

      expect(result).toBeNull();
    }
  );

  it("should return null if pipeline not started", async () => {
    const pipeline = new Pipeline("videotestsrc ! videoconvert ! appsink name=sink");
    const sink = pipeline.getElementByName("sink");

    if (sink?.type !== "app-sink-element") throw new Error("Expected app sink element");

    const result = await sink.getSample();

    pipeline.stop();

    expect(result).toBeNull();
  });

  it("should pull expected number of frames", async () => {
    const frames = 5;
    const pipeline = new Pipeline(
      `videotestsrc num-buffers=${frames} ! videoconvert ! appsink name=sink`
    );
    const sink = pipeline.getElementByName("sink");

    if (sink?.type !== "app-sink-element") throw new Error("Expected app sink element");

    await pipeline.play();

    const samples: GStreamerSample[] = [];

    let currFrames = 0;
    while (true) {
      const result = await sink.getSample();
      if (!result) break;
      samples.push(result);
      currFrames++;
    }

    pipeline.stop();

    expect(currFrames).toBe(frames);
  });

  it("should receive samples via onSample event callback", async () => {
    const frames = 3;
    const pipeline = new Pipeline(
      `videotestsrc num-buffers=${frames} ! videoconvert ! appsink name=sink`
    );
    const sink = pipeline.getElementByName("sink");

    if (sink?.type !== "app-sink-element") throw new Error("Expected app sink element");

    const samples: GStreamerSample[] = [];

    const unsubscribe: () => void = await new Promise(resolve => {
      const unsubscribe = sink.onSample((sample: GStreamerSample) => {
        samples.push(sample);
        if (samples.length === frames) {
          resolve(unsubscribe);
        }
      });

      pipeline.play();
    });

    unsubscribe();
    pipeline.stop();

    expect(samples).toHaveLength(frames);
    expect(samples[0].buffer).toBeDefined();
  });
});
