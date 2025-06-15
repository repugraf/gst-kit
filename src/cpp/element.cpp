#include "element.hpp"
#include "constructor_registry.hpp"
#include <thread>

// ElementBase Implementation
void ElementBase::initializeElement(const Napi::CallbackInfo &info) {
  if (info.Length() > 0 && info[0].IsExternal()) {
    GstElement *elem = info[0].As<Napi::External<GstElement>>().Data();
    element.reset(elem);
  }
}

// Element Implementation
Napi::Object Element::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "Element", {});
  exports.Set("Element", func);
  ConstructorRegistry::RegisterConstructor(env, "Element", func);
  return exports;
}

Napi::Object Element::CreateFromGstElement(Napi::Env env, GstElement *element) {
  std::string constructorName;
  
  if (GST_IS_APP_SINK(element)) {
    constructorName = "AppSinkElement";
  } else if (GST_IS_APP_SRC(element)) {
    constructorName = "AppSrcElement";
  } else {
    constructorName = "Element";
  }
  
  // Get the stored constructor from registry
  if (ConstructorRegistry::HasConstructor(env, constructorName)) {
    Napi::Function constructor = ConstructorRegistry::GetConstructor(env, constructorName);
    return constructor.New({Napi::External<GstElement>::New(env, element)});
  }
  
  // Fallback - should not happen in normal operation
  Napi::TypeError::New(env, "Constructor not found for element type").ThrowAsJavaScriptException();
  return env.Undefined().As<Napi::Object>();
}

Element::Element(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Element>(info) {
  initializeElement(info);
}

// AsyncWorker for pulling samples with timeout
class PullSampleWorker : public Napi::AsyncWorker {
public:
  PullSampleWorker(Napi::Env env, GstAppSink *appSink, guint64 timeoutMs) :
      Napi::AsyncWorker(env), appSink(appSink), timeoutMs(timeoutMs), sample(nullptr),
      deferred(env) {
    // Increase reference count since we'll be using this in another thread
    gst_object_ref(appSink);
  }

  ~PullSampleWorker() { cleanup(); }

  void Execute() override {
    // Convert timeout from milliseconds to nanoseconds (GstClockTime)
    GstClockTime timeout = timeoutMs * GST_MSECOND;

    // Use GStreamer's built-in timeout mechanism
    sample = gst_app_sink_try_pull_sample(appSink, timeout);
    // sample will be NULL if timeout expires or on EOS/error
  }

  Napi::Promise::Deferred GetPromise() { return deferred; }

  void OnOK() override {
    Napi::HandleScope scope(Env());

    if (sample) {
      // Extract buffer data from sample
      GstBuffer *buffer = gst_sample_get_buffer(sample);
      if (buffer) {
        GstMapInfo mapInfo;
        if (gst_buffer_map(buffer, &mapInfo, GST_MAP_READ)) {
          // Create Node.js Buffer from GStreamer buffer data
          Napi::Buffer<uint8_t> nodeBuffer =
            Napi::Buffer<uint8_t>::Copy(Env(), mapInfo.data, mapInfo.size);
          gst_buffer_unmap(buffer, &mapInfo);

          deferred.Resolve(nodeBuffer);
          return;
        }
      }
    }

    // Timeout or no sample/error
    deferred.Resolve(Env().Null());
  }

  void OnError(const Napi::Error &error) override {
    Napi::HandleScope scope(Env());
    deferred.Reject(error.Value());
  }

private:
  void cleanup() {
    // Clean up resources - safe to call multiple times
    if (sample) {
      gst_sample_unref(sample);
      sample = nullptr;
    }
    if (appSink) {
      gst_object_unref(appSink);
      appSink = nullptr;
    }
  }

  GstAppSink *appSink;
  guint64 timeoutMs;
  GstSample *sample;
  Napi::Promise::Deferred deferred;
};

// AppSinkElement Implementation
Napi::Object AppSinkElement::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "AppSinkElement", {});
  exports.Set("AppSinkElement", func);
  ConstructorRegistry::RegisterConstructor(env, "AppSinkElement", func);
  return exports;
}

AppSinkElement::AppSinkElement(const Napi::CallbackInfo &info) :
    Napi::ObjectWrap<AppSinkElement>(info) {
  initializeElement(info);
  
  // Add methods as enumerable instance properties to make them visible in console.log
  Napi::Env env = info.Env();
  Napi::Object thisObj = info.This().As<Napi::Object>();
  
  // Create bound method
  auto pull_method = Napi::Function::New(env, [this](const Napi::CallbackInfo &info) -> Napi::Value {
    return this->pull(info);
  }, "pull");
  
  thisObj.DefineProperties({
    Napi::PropertyDescriptor::Value("pull", pull_method, napi_enumerable)
  });
}

Napi::Value AppSinkElement::pull(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Validate that we have an app sink element
  if (!element || !GST_IS_APP_SINK(element.get())) {
    Napi::TypeError::New(env, "Invalid AppSink element").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  // Default timeout is 1000ms (1 second)
  guint64 timeoutMs = 1000;

  // Check if timeout parameter is provided
  if (info.Length() > 0 && info[0].IsNumber()) {
    timeoutMs = info[0].As<Napi::Number>().Uint32Value();
  }

  // Create worker and get its promise
  // Note: N-API AsyncWorker manages its own memory - it will be automatically
  // deleted when the work completes (OnOK or OnError is called)
  PullSampleWorker *worker = new PullSampleWorker(env, GST_APP_SINK(element.get()), timeoutMs);
  Napi::Promise promise = worker->GetPromise().Promise();
  worker->Queue();

  return promise;
}

// AppSrcElement Implementation
Napi::Object AppSrcElement::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "AppSrcElement", {});
  exports.Set("AppSrcElement", func);
  ConstructorRegistry::RegisterConstructor(env, "AppSrcElement", func);
  return exports;
}

AppSrcElement::AppSrcElement(const Napi::CallbackInfo &info) :
    Napi::ObjectWrap<AppSrcElement>(info) {
  initializeElement(info);
}
