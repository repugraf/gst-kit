#include "pipeline.hpp"
#include "async-workers.hpp"
#include "element.hpp"
#include <gst/gst.h>
#include <gst/video/video.h>

Napi::Object Pipeline::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "Pipeline", {});
  exports.Set("Pipeline", func);
  return exports;
}

Pipeline::Pipeline(const Napi::CallbackInfo &info) :
    Napi::ObjectWrap<Pipeline>(info), pipeline(nullptr, gst_object_unref) {
  gst_init(NULL, NULL);
  Napi::Env env = info.Env();
  GError *err = NULL;

  if (info.Length() > 0 && info[0].IsString()) {
    pipeline_string = info[0].As<Napi::String>().Utf8Value();
  } else {
    Napi::Error::New(env, "Wrong type value for pipeline string").ThrowAsJavaScriptException();
  }

  GstPipeline *raw_pipeline =
    (GstPipeline *)GST_BIN(gst_parse_launch(pipeline_string.c_str(), &err));
  if (err) {
    Napi::Error::New(env, err->message).ThrowAsJavaScriptException();
  }

  pipeline.reset(raw_pipeline);

  // Set methods as enumerable instance properties to make them visible in console.log
  Napi::Object thisObj = info.This().As<Napi::Object>();

  // Create bound methods
  auto play_method = Napi::Function::New(
    env, [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->play(info); }, "play"
  );
  auto pause_method = Napi::Function::New(
    env, [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->pause(info); },
    "pause"
  );
  auto stop_method = Napi::Function::New(
    env, [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->stop(info); }, "stop"
  );
  auto playing_method = Napi::Function::New(
    env, [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->playing(info); },
    "playing"
  );
  auto get_element_by_name_method = Napi::Function::New(
    env,
    [this](const Napi::CallbackInfo &info) -> Napi::Value {
      return this->get_element_by_name(info);
    },
    "getElementByName"
  );
  auto queryPosition_method = Napi::Function::New(
    env,
    [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->query_position(info); },
    "queryPosition"
  );
  auto queryDuration_method = Napi::Function::New(
    env,
    [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->query_duration(info); },
    "queryDuration"
  );
  auto busPop_method = Napi::Function::New(
    env, [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->bus_pop(info); },
    "busPop"
  );
  auto seek_method = Napi::Function::New(
    env, [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->seek(info); }, "seek"
  );

  thisObj.DefineProperties(
    {Napi::PropertyDescriptor::Value("play", play_method, napi_enumerable),
     Napi::PropertyDescriptor::Value("pause", pause_method, napi_enumerable),
     Napi::PropertyDescriptor::Value("stop", stop_method, napi_enumerable),
     Napi::PropertyDescriptor::Value("playing", playing_method, napi_enumerable),
     Napi::PropertyDescriptor::Value(
       "getElementByName", get_element_by_name_method, napi_enumerable
     ),
     Napi::PropertyDescriptor::Value("queryPosition", queryPosition_method, napi_enumerable),
     Napi::PropertyDescriptor::Value("queryDuration", queryDuration_method, napi_enumerable),
     Napi::PropertyDescriptor::Value("busPop", busPop_method, napi_enumerable),
     Napi::PropertyDescriptor::Value("seek", seek_method, napi_enumerable)}
  );
}

Napi::Value Pipeline::play(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Default timeout is 1000ms (1 second)
  GstClockTime timeout = 1000 * GST_MSECOND;

  // Check if timeout parameter is provided
  if (info.Length() > 0 && info[0].IsNumber()) {
    double timeout_ms = info[0].As<Napi::Number>().DoubleValue();
    if (timeout_ms < 0) {
      // Negative timeout means infinite wait
      timeout = GST_CLOCK_TIME_NONE;
    } else {
      timeout = static_cast<GstClockTime>(timeout_ms * GST_MSECOND);
    }
  }

  // Create worker and get its promise
  StateChangeWorker *worker =
    new StateChangeWorker(env, pipeline.get(), GST_STATE_PLAYING, timeout);
  Napi::Promise promise = worker->GetPromise().Promise();
  worker->Queue();

  return promise;
}

Napi::Value Pipeline::pause(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Default timeout is 1000ms (1 second)
  GstClockTime timeout = 1000 * GST_MSECOND;

  // Check if timeout parameter is provided
  if (info.Length() > 0 && info[0].IsNumber()) {
    double timeout_ms = info[0].As<Napi::Number>().DoubleValue();
    if (timeout_ms < 0) {
      // Negative timeout means infinite wait
      timeout = GST_CLOCK_TIME_NONE;
    } else {
      timeout = static_cast<GstClockTime>(timeout_ms * GST_MSECOND);
    }
  }

  // Create worker and get its promise
  StateChangeWorker *worker = new StateChangeWorker(env, pipeline.get(), GST_STATE_PAUSED, timeout);
  Napi::Promise promise = worker->GetPromise().Promise();
  worker->Queue();

  return promise;
}

Napi::Value Pipeline::stop(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Default timeout is 1000ms (1 second)
  GstClockTime timeout = 1000 * GST_MSECOND;

  // Check if timeout parameter is provided
  if (info.Length() > 0 && info[0].IsNumber()) {
    double timeout_ms = info[0].As<Napi::Number>().DoubleValue();
    if (timeout_ms < 0) {
      // Negative timeout means infinite wait
      timeout = GST_CLOCK_TIME_NONE;
    } else {
      timeout = static_cast<GstClockTime>(timeout_ms * GST_MSECOND);
    }
  }

  // Create worker and get its promise
  StateChangeWorker *worker = new StateChangeWorker(env, pipeline.get(), GST_STATE_NULL, timeout);
  Napi::Promise promise = worker->GetPromise().Promise();
  worker->Queue();

  return promise;
}

Napi::Value Pipeline::get_element_by_name(const Napi::CallbackInfo &info) {
  auto name = info[0].As<Napi::String>().Utf8Value();
  GstElement *e = gst_bin_get_by_name(GST_BIN(pipeline.get()), name.c_str());

  if (e == nullptr) return info.Env().Null();

  // Use the stored constructors to create the appropriate element
  return Element::CreateFromGstElement(info.Env(), e);
}

Napi::Value Pipeline::playing(const Napi::CallbackInfo &info) {
  GstState state;
  GstState pending;
  GstStateChangeReturn ret =
    gst_element_get_state(GST_ELEMENT(pipeline.get()), &state, &pending, 5 * GST_MSECOND);

  // If state change is in progress and we're transitioning to PLAYING, consider it as playing
  bool is_playing =
    (state == GST_STATE_PLAYING) || (ret == GST_STATE_CHANGE_ASYNC && pending == GST_STATE_PLAYING);

  return Napi::Boolean::New(info.Env(), is_playing);
}

Napi::Value Pipeline::query_position(const Napi::CallbackInfo &info) {
  gint64 pos;
  gst_element_query_position(GST_ELEMENT(pipeline.get()), GST_FORMAT_TIME, &pos);
  double r = pos == -1 ? -1 : (double)pos / GST_SECOND;
  return Napi::Number::New(info.Env(), r);
}

Napi::Value Pipeline::query_duration(const Napi::CallbackInfo &info) {
  gint64 dur;
  gst_element_query_duration(GST_ELEMENT(pipeline.get()), GST_FORMAT_TIME, &dur);
  double r = dur == -1 ? -1 : (double)dur / GST_SECOND;
  return Napi::Number::New(info.Env(), r);
}

Napi::Value Pipeline::bus_pop(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Default timeout is 1000ms (1 second) - converted to nanoseconds
  GstClockTime timeout = 1000 * GST_MSECOND;

  // Check if timeout parameter is provided
  if (info.Length() > 0 && info[0].IsNumber()) {
    double timeout_ms = info[0].As<Napi::Number>().DoubleValue();
    if (timeout_ms < 0) {
      // Negative timeout means infinite wait
      timeout = GST_CLOCK_TIME_NONE;
    } else {
      timeout = static_cast<GstClockTime>(timeout_ms * GST_MSECOND);
    }
  }

  // Create worker and get its promise
  BusPopWorker *worker = new BusPopWorker(env, pipeline.get(), timeout);
  Napi::Promise promise = worker->GetPromise().Promise();
  worker->Queue();

  return promise;
}

Napi::Value Pipeline::seek(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !info[0].IsNumber()) {
    Napi::TypeError::New(env, "seek() requires a number argument (position in seconds)")
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  double position_seconds = info[0].As<Napi::Number>().DoubleValue();

  if (position_seconds < 0) {
    Napi::TypeError::New(env, "Position must be >= 0").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  // Convert seconds to nanoseconds
  GstClockTime position_ns = static_cast<GstClockTime>(position_seconds * GST_SECOND);

  // Perform the seek
  gboolean result = gst_element_seek(
    GST_ELEMENT(pipeline.get()),
    1.0,                 // Rate (1.0 = normal speed)
    GST_FORMAT_TIME,     // Format (time-based seeking)
    GST_SEEK_FLAG_FLUSH, // Flags (flush pipeline)
    GST_SEEK_TYPE_SET,   // Start type (absolute position)
    position_ns,         // Start position
    GST_SEEK_TYPE_NONE,  // Stop type (no stop position)
    GST_CLOCK_TIME_NONE  // Stop position (unused)
  );

  return Napi::Boolean::New(env, result);
}