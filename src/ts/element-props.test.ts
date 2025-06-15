import { describe, it, expect } from "vitest";
import { Pipeline } from "./";

describe("Element Properties", () => {
  it("should get prop value", () => {
    const caps = "video/x-raw,format=(string)GRAY8";
    const pipeline = new Pipeline(
      `videotestsrc ! capsfilter name=src caps=${caps} ! videoconvert ! appsink`
    );
    const element = pipeline.getElementByName("src");

    if (!element) throw new Error("Element not found");

    const prop = element.getElementProperty("caps");

    caps.split(",").forEach(cap => expect(prop).toContain(cap));
  });
});
