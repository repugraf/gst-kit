import { describe, expect, it } from "vitest";
import { Pipeline, AppSinkElement } from ".";

describe("AppSink", () => {
  it("should pull frames", async () => {
    const pipeline = new Pipeline("videotestsrc ! videoconvert ! appsink name=sink");
    const sink = pipeline.getElementByName("sink");

    if (!(sink instanceof AppSinkElement)) throw new Error("Expected app sink element");

    pipeline.play();

    const buffer = await sink.pull();

    pipeline.stop();

    expect(buffer).not.toBeNull();
  });
});
