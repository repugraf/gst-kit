import { describe, it, expect, vi } from "vitest";
import {Pipeline} from "./index";

describe("appsink", () => {
    it("should work with appsink", () => {
        const pipeline = new Pipeline('videotestsrc num-buffers=15 ! appsink name=sink');
        const sink = pipeline.findChild('sink');
        
        const onPull =  vi.fn((buf: any) =>  !!buf);

        pipeline.play();
        sink.pull(onPull);

        expect(onPull).toHaveBeenCalled();
        expect(onPull).toReturnWith(true);
    });
});