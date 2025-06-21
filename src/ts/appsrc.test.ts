import { describe, expect, it } from "vitest";
import { Pipeline } from ".";

describe.concurrent("AppSrc", () => {
  it("should push buffer to app source", () => {
    const pipeline = new Pipeline("appsrc name=source ! fakesink");
    const source = pipeline.getElementByName("source");

    if (source?.type !== "app-src-element") throw new Error("Expected app source element");

    // Create a test buffer with some data
    const testData = Buffer.from([1, 2, 3, 4, 5]);

    // Should not throw an error when pushing buffer
    expect(() => {
      source.push(testData);
    }).not.toThrow();
  });

  it("should push buffer with PTS as number", () => {
    const pipeline = new Pipeline("appsrc name=source ! fakesink");
    const source = pipeline.getElementByName("source");

    if (source?.type !== "app-src-element") throw new Error("Expected app source element");

    const testData = Buffer.from([1, 2, 3, 4, 5]);
    const pts = 1000000000; // 1 second in nanoseconds

    expect(() => {
      source.push(testData, pts);
    }).not.toThrow();
  });

  it("should push buffer with PTS as Buffer (legacy compatibility)", () => {
    const pipeline = new Pipeline("appsrc name=source ! fakesink");
    const source = pipeline.getElementByName("source");

    if (source?.type !== "app-src-element") throw new Error("Expected app source element");

    const testData = Buffer.from([1, 2, 3, 4, 5]);
    // Create PTS buffer in big-endian format (8 bytes)
    const ptsBuffer = Buffer.alloc(8);
    ptsBuffer.writeBigUInt64BE(BigInt(1000000000), 0); // 1 second

    expect(() => {
      source.push(testData, ptsBuffer);
    }).not.toThrow();
  });

  it("should throw error when push called on non-app-src element", () => {
    const pipeline = new Pipeline("videotestsrc name=source ! fakesink");
    const source = pipeline.getElementByName("source");

    if (!source) throw new Error("Element not found");

    const testData = Buffer.from([1, 2, 3, 4, 5]);

    expect(() => {
      (source as any).push(testData);
    }).toThrow("push is not a function");
  });

  it("should throw error when buffer argument is missing", () => {
    const pipeline = new Pipeline("appsrc name=source ! fakesink");
    const source = pipeline.getElementByName("source");

    if (source?.type !== "app-src-element") throw new Error("Expected app source element");

    expect(() => {
      (source as any).push();
    }).toThrow("push() requires at least 1 argument: buffer");
  });

  it("should throw error when buffer argument is not a Buffer", () => {
    const pipeline = new Pipeline("appsrc name=source ! fakesink");
    const source = pipeline.getElementByName("source");

    if (source?.type !== "app-src-element") throw new Error("Expected app source element");

    expect(() => {
      (source as any).push("not a buffer");
    }).toThrow("First argument must be a Buffer");
  });
});
