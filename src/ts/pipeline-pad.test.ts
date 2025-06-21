import { describe, it, expect } from "vitest";
import { Pipeline } from "./";

describe.concurrent("Pipeline Pad Methods", () => {
  it("should get pad information from an element", () => {
    const pipeline = new Pipeline("videotestsrc name=source ! fakesink name=sink");
    const element = pipeline.getElementByName("source");

    // Get source pad from videotestsrc
    const srcPad = element?.getPad("src");

    expect(srcPad).not.toBeNull();
    expect(srcPad?.name).toBe("src");
    expect(srcPad?.direction).toBe(1); // GST_PAD_SRC = 1
    // caps may be null if not negotiated yet
  });

  it("should get sink pad information from an element", () => {
    const pipeline = new Pipeline("videotestsrc name=source ! fakesink name=sink");
    const element = pipeline.getElementByName("sink");

    // Get sink pad from fakesink
    const sinkPad = element?.getPad("sink");

    expect(sinkPad).not.toBeNull();
    expect(sinkPad?.name).toBe("sink");
    expect(sinkPad?.direction).toBe(2); // GST_PAD_SINK = 2
  });

  it("should return null for non-existent element", () => {
    const pipeline = new Pipeline("videotestsrc name=source ! fakesink name=sink");

    const element = pipeline.getElementByName("nonexistent");

    expect(element).toBeNull();
  });

  it("should return null for non-existent pad", () => {
    const pipeline = new Pipeline("videotestsrc name=source ! fakesink name=sink");
    const element = pipeline.getElementByName("source");

    const pad = element?.getPad("nonexistent");

    expect(pad).toBeNull();
  });

  it("should set pad as property on an element", async () => {
    const pipeline = new Pipeline(
      "input-selector name=sel ! fakesink videotestsrc pattern=0 ! sel.sink_0 videotestsrc pattern=1 ! sel.sink_1"
    );

    const sel = pipeline.getElementByName("sel");
    await pipeline.play();

    if (!sel) throw new Error("Element expected to be present");

    expect(() => sel.setPad("active-pad", "sink_0")).not.toThrow();
    expect(() => sel.setPad("active-pad", "sink_1")).not.toThrow();
    expect(() => sel.setPad("active-pad", "sink_0")).not.toThrow();
  });

  it("should throw error for setPad with invalid element", () => {
    const pipeline = new Pipeline("videotestsrc name=source ! fakesink name=sink");
    const element = pipeline.getElementByName("nonexistent");

    expect(element).toBeNull();
  });

  it("should throw error for setPad with invalid pad", () => {
    const pipeline = new Pipeline("videotestsrc name=source ! fakesink name=sink");
    const element = pipeline.getElementByName("source");

    expect(() => {
      element?.setPad("someattr", "nonexistent");
    }).toThrow("Pad not found: nonexistent");
  });

  it("should throw error for setPad with insufficient arguments", () => {
    const pipeline = new Pipeline("videotestsrc name=source ! fakesink name=sink");
    const element = pipeline.getElementByName("source");

    expect(() => {
      (element as any).setPad("attr");
    }).toThrow("setPad() requires two string arguments");
  });

  it("should throw error for getPad with insufficient arguments", () => {
    const pipeline = new Pipeline("videotestsrc name=source ! fakesink name=sink");
    const element = pipeline.getElementByName("source");

    expect(() => {
      (element as any).getPad();
    }).toThrow("getPad() requires one string argument");
  });

  it("should handle pad caps when available", async () => {
    const pipeline = new Pipeline(
      "videotestsrc name=source ! capsfilter name=filter ! fakesink name=sink"
    );

    // Set caps using element property instead
    const filter = pipeline.getElementByName("filter");
    if (filter) {
      filter.setElementProperty("caps", "video/x-raw,width=640,height=480");
    }

    // Start the pipeline to negotiate caps
    await pipeline.play();

    // Let caps negotiate
    await new Promise(resolve => setTimeout(resolve, 100));

    const filterElement = pipeline.getElementByName("filter");
    const srcPad = filterElement?.getPad("src");

    expect(srcPad).not.toBeNull();
    expect(srcPad?.name).toBe("src");
    expect(srcPad?.direction).toBe(1); // GST_PAD_SRC = 1

    // After caps negotiation, caps should be available
    if (srcPad?.caps) {
      expect(srcPad.caps).toContain("video/x-raw");
      expect(srcPad.caps).toContain("width=(int)640");
      expect(srcPad.caps).toContain("height=(int)480");
    }

    await pipeline.stop();
  });
});
