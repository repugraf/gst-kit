import { describe, expect, it } from "vitest";
import { Pipeline } from ".";

describe("AppSink", () => {
  it("should pull frames", async () => {
    const pipeline = new Pipeline("videotestsrc ! videoconvert ! appsink name=sink");
    const sink = pipeline.getElementByName("sink");

    if (sink?.type !== "app-sink-element") throw new Error("Expected app sink element");

    pipeline.play();

    const buffer = await sink.pull();

    pipeline.stop();

    expect(buffer).not.toBeNull();
  });

  it("should return null on pull if no frames received", async () => {
    const pipeline = new Pipeline(
      "udpsrc address=127.0.0.1 port=5004 ! queue " +
        "! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! appsink name=sink"
    );
    const sink = pipeline.getElementByName("sink");

    if (sink?.type !== "app-sink-element") throw new Error("Expected app sink element");

    pipeline.play();

    const buffer = await sink.pull(10);

    pipeline.stop();

    expect(buffer).toBeNull();
  });

  it("should return null if pipeline not started", async () => {
    const pipeline = new Pipeline("videotestsrc ! videoconvert ! appsink name=sink");
    const sink = pipeline.getElementByName("sink");

    if (sink?.type !== "app-sink-element") throw new Error("Expected app sink element");

    const buffer = await sink.pull();

    pipeline.stop();

    expect(buffer).toBeNull();
  });

  it("should pull expected number of frames", async () => {
    const frames = 5;
    const pipeline = new Pipeline(
      `videotestsrc num-buffers=${frames} ! videoconvert ! appsink name=sink`
    );
    const sink = pipeline.getElementByName("sink");

    if (sink?.type !== "app-sink-element") throw new Error("Expected app sink element");

    pipeline.play();

    let currFrames = 0;
    while (true) {
      const buffer = await sink.pull();
      if (!buffer) break;
      currFrames++;
    }

    pipeline.stop();

    expect(currFrames).toBe(frames);
  });
});
