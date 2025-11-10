import { describe, it, expect } from "vitest";
import { Pipeline } from "./index.js";

describe("Pipeline.elementExists", () => {
  it("should return true for built-in elements that exist", () => {
    // Test commonly available GStreamer elements
    expect(Pipeline.elementExists("videotestsrc")).toBe(true);
    expect(Pipeline.elementExists("audiotestsrc")).toBe(true);
    expect(Pipeline.elementExists("fakesink")).toBe(true);
    expect(Pipeline.elementExists("identity")).toBe(true);
    expect(Pipeline.elementExists("queue")).toBe(true);
  });

  it("should return false for non-existent elements", () => {
    expect(Pipeline.elementExists("nonexistentelement123")).toBe(false);
    expect(Pipeline.elementExists("fakeelement999")).toBe(false);
    expect(Pipeline.elementExists("notarealelement")).toBe(false);
  });

  it("should return true for appsrc and appsink", () => {
    expect(Pipeline.elementExists("appsrc")).toBe(true);
    expect(Pipeline.elementExists("appsink")).toBe(true);
  });

  it("should handle empty string", () => {
    expect(Pipeline.elementExists("")).toBe(false);
  });

  it("should handle common plugin elements", () => {
    // These might not be available on all systems, but the method should work
    const elements = [
      "x264enc",
      "vp8enc",
      "h264parse",
      "videoconvert",
      "audioresample",
      "autoaudiosink",
      "autovideosink",
    ];

    elements.forEach(element => {
      const exists = Pipeline.elementExists(element);
      // Just verify it returns a boolean, don't assert the value
      expect(typeof exists).toBe("boolean");
    });
  });

  it("should be case-sensitive", () => {
    // GStreamer element names are case-sensitive
    expect(Pipeline.elementExists("videotestsrc")).toBe(true);
    expect(Pipeline.elementExists("VideoTestSrc")).toBe(false);
    expect(Pipeline.elementExists("VIDEOTESTSRC")).toBe(false);
  });

  it("should handle special characters in element names", () => {
    // Test with invalid characters that shouldn't match any element
    expect(Pipeline.elementExists("video-test-src!@#")).toBe(false);
    expect(Pipeline.elementExists("element with spaces")).toBe(false);
  });
});
