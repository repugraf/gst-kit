#include <napi.h>
#include "Pipeline.h"
#include "GObjectWrap.h"

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    // Initialize GStreamer
    gst_init(nullptr, nullptr);

    // Register the Pipeline class
    Pipeline::Init(env, exports);
    
    // Register the GObjectWrap class
    GObjectWrap::Init(env, exports);

    return exports;
}

NODE_API_MODULE(native_addon, Init)
