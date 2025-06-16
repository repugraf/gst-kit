#include "element.hpp"
#include "type-conversion.hpp"
#include <thread>

Napi::Object Element::CreateFromGstElement(Napi::Env env, GstElement *element) {
  Napi::Function func = DefineClass(env, "Element", {});
  return func.New({Napi::External<GstElement>::New(env, element)});
}

Element::Element(const Napi::CallbackInfo &info) :
    Napi::ObjectWrap<Element>(info), element(nullptr, gst_object_unref) {
  std::string element_type = "element";
  if (info.Length() > 0 && info[0].IsExternal()) {
    GstElement *elem = info[0].As<Napi::External<GstElement>>().Data();
    element.reset(elem);

    // Set element type based on GStreamer element type
    if (GST_IS_APP_SINK(elem))
      element_type = "app-sink-element";
    else if (GST_IS_APP_SRC(elem))
      element_type = "app-src-element";
  }

  // Set properties as enumerable instance properties
  Napi::Env env = info.Env();
  Napi::Object thisObj = info.This().As<Napi::Object>();

  auto pull_method = Napi::Function::New(
    env, [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->pull(info); }, "pull"
  );
  auto get_element_property_method = Napi::Function::New(
    env,
    [this](const Napi::CallbackInfo &info) -> Napi::Value {
      return this->get_element_property(info);
    },
    "getElementProperty"
  );
  auto set_element_property_method = Napi::Function::New(
    env,
    [this](const Napi::CallbackInfo &info) -> Napi::Value {
      return this->set_element_property(info);
    },
    "setElementProperty"
  );

  std::vector property_descriptors = {
    Napi::PropertyDescriptor::Value("type", Napi::String::New(env, element_type), napi_enumerable),
    Napi::PropertyDescriptor::Value(
      "getElementProperty", get_element_property_method, napi_enumerable
    ),
    Napi::PropertyDescriptor::Value(
      "setElementProperty", set_element_property_method, napi_enumerable
    )
  };

  if (element || GST_IS_APP_SINK(element.get()))
    property_descriptors.push_back(
      Napi::PropertyDescriptor::Value("pull", pull_method, napi_enumerable)
    );

  thisObj.DefineProperties(property_descriptors);
}

Napi::Value Element::get_element_property(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Property name must be a string").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (!element.get()) {
    Napi::TypeError::New(env, "Element is null or not initialized").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string property_name = info[0].As<Napi::String>().Utf8Value();

  GParamSpec *spec =
    g_object_class_find_property(G_OBJECT_GET_CLASS(element.get()), property_name.c_str());

  if (!spec) {
    return env.Null();
  }

  GValue value = G_VALUE_INIT;
  g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(spec));
  g_object_get_property(G_OBJECT(element.get()), property_name.c_str(), &value);

  Napi::Value result = TypeConversion::gvalue_to_js(env, &value);

  g_value_unset(&value);
  return result;
}

Napi::Value Element::set_element_property(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "setElementProperty requires property name and value").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  
  if (!info[0].IsString()) {
    Napi::TypeError::New(env, "Property name must be a string").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (!element.get()) {
    Napi::TypeError::New(env, "Element is null or not initialized").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string property_name = info[0].As<Napi::String>().Utf8Value();
  Napi::Value property_value = info[1];

  GParamSpec *spec =
    g_object_class_find_property(G_OBJECT_GET_CLASS(element.get()), property_name.c_str());

  if (!spec) {
    Napi::TypeError::New(env, ("Property '" + property_name + "' not found").c_str()).ThrowAsJavaScriptException();
    return env.Undefined();
  }

  // Check if property is writable
  if (!(spec->flags & G_PARAM_WRITABLE)) {
    Napi::TypeError::New(env, ("Property '" + property_name + "' is not writable").c_str()).ThrowAsJavaScriptException();
    return env.Undefined();
  }

  GValue value = G_VALUE_INIT;
  GType prop_type = G_PARAM_SPEC_VALUE_TYPE(spec);
  
  // Convert JavaScript value to GValue
  if (!TypeConversion::js_to_gvalue(env, property_value, prop_type, &value)) {
    std::string error_msg = TypeConversion::get_conversion_error_message(prop_type, property_value);
    
    // Handle special error cases
    if (prop_type == gst_caps_get_type() && property_value.IsString()) {
      error_msg = "Invalid caps string";
    } else if (G_TYPE_IS_ENUM(prop_type) && property_value.IsString()) {
      std::string enum_str = property_value.As<Napi::String>().Utf8Value();
      error_msg = "Invalid enum value: " + enum_str;
    }
    
    Napi::TypeError::New(env, error_msg.c_str()).ThrowAsJavaScriptException();
    return env.Undefined();
  }

  // Set the property
  g_object_set_property(G_OBJECT(element.get()), property_name.c_str(), &value);
  g_value_unset(&value);
  
  return env.Undefined();
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

Napi::Value Element::pull(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Validate that we have an app sink element
  if (!element || !GST_IS_APP_SINK(element.get())) {
    Napi::TypeError::New(env, "pull() can only be called on app-sink-element")
      .ThrowAsJavaScriptException();
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
