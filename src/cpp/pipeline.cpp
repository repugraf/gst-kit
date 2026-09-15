#include "pipeline.hpp"
#include "async-workers.hpp"
#include "element.hpp"
#include <gst/gst.h>
#include <gst/video/video.h>

bool Pipeline::gst_initialized = false;

void Pipeline::ensure_gst_initialized() {
  if (!gst_initialized) {
    gst_init(NULL, NULL);
    gst_initialized = true;
  }
}

Napi::Object Pipeline::Init(const Napi::Env &env, const Napi::Object &exports) {
  Napi::Function func = DefineClass(env, "Pipeline", {});

  func.Set("elementExists", Napi::Function::New(env, Pipeline::ElementExists, "elementExists"));

  exports.Set("Pipeline", func);
  return exports;
}

Pipeline::Pipeline(const Napi::CallbackInfo &info) :
    Napi::ObjectWrap<Pipeline>(info), pipeline(nullptr, gst_object_unref), disposed(false),
    in_flight_state_changes(0) {
  ensure_gst_initialized();
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
  auto end_of_stream_method = Napi::Function::New(
    env,
    [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->end_of_stream(info); },
    "endOfStream"
  );
  auto dispose_method = Napi::Function::New(
    env,
    [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->dispose(info); },
    "dispose"
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
     Napi::PropertyDescriptor::Value("seek", seek_method, napi_enumerable),
     Napi::PropertyDescriptor::Value("endOfStream", end_of_stream_method, napi_enumerable),
     Napi::PropertyDescriptor::Value("dispose", dispose_method, napi_enumerable)}
  );
}

GstPipeline *Pipeline::require_pipeline(const Napi::Env &env) {
  // `disposed` is the authoritative sentinel, not a null `pipeline`: when
  // dispose() runs while a state-change worker is in flight the native teardown
  // is deferred, so the pointer can still be non-null after dispose() returns.
  // Guarding on the flag keeps use-after-dispose loud in that window too.
  if (disposed || pipeline.get() == nullptr) {
    Napi::Error::New(env, "Pipeline used after dispose()").ThrowAsJavaScriptException();
    return nullptr;
  }
  return pipeline.get();
}

void Pipeline::release_native_pipeline() {
  GstPipeline *raw = pipeline.get();
  if (raw == nullptr) return;

  // The native pipeline must reach NULL before its reference is dropped:
  // gst_object_unref on a non-NULL pipeline leaks its pads, bus, clock, and the
  // elements' internal buffers (and can emit a g_critical). Callers stop()
  // first, so this is normally already NULL. But stop() is bounded by a timeout
  // and its NULL transition may not have fully settled — so rather than skip the
  // unref (which guarantees the leak we are trying to prevent), force the
  // pipeline to NULL here, synchronously, and then release. Setting an
  // already-NULL pipeline to NULL is a cheap no-op, so the common stopped-first
  // path pays almost nothing; the blocking transition only happens on the rare
  // not-fully-stopped path, where it is exactly what prevents the leak.
  GstState state;
  GstState pending;
  gst_element_get_state(GST_ELEMENT(raw), &state, &pending, 0);
  if (state != GST_STATE_NULL || pending != GST_STATE_VOID_PENDING) {
    gst_element_set_state(GST_ELEMENT(raw), GST_STATE_NULL);
    // Block until the NULL transition completes so the unref below finalizes a
    // truly-NULL pipeline. NULL is reached synchronously for virtually all
    // pipelines; the wait is bounded so a pathological element cannot hang here.
    gst_element_get_state(GST_ELEMENT(raw), &state, &pending, 5 * GST_SECOND);
  }

  // Drop our reference and let unique_ptr's deleter (gst_object_unref) run.
  pipeline.reset();
}

void Pipeline::state_worker_started() {
  // Keep the JS wrapper alive for as long as a worker holds a back-pointer to
  // this Pipeline, so the worker's OnOK/OnError can safely call back into it
  // even if all JS references are dropped while the state change is in flight.
  if (in_flight_state_changes == 0) Ref();
  in_flight_state_changes++;
}

void Pipeline::state_worker_finished() {
  if (in_flight_state_changes > 0) in_flight_state_changes--;

  // If dispose() was requested while workers were running, the last worker to
  // finish performs the deferred native teardown. By now no state change can
  // drive the pipeline back up, so forcing NULL and releasing is safe.
  if (disposed && in_flight_state_changes == 0) {
    release_native_pipeline();
  }

  // Balance the Ref() taken in state_worker_started() once the last worker is
  // done. This may finalize the wrapper, so touch no members afterward.
  if (in_flight_state_changes == 0) Unref();
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

  GstPipeline *raw = require_pipeline(env);
  if (raw == nullptr) return env.Undefined();

  // Create worker and get its promise
  StateChangeWorker *worker = new StateChangeWorker(env, this, raw, GST_STATE_PLAYING, timeout);
  Napi::Promise promise = worker->GetPromise().Promise();
  state_worker_started();
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

  GstPipeline *raw = require_pipeline(env);
  if (raw == nullptr) return env.Undefined();

  // Create worker and get its promise
  StateChangeWorker *worker = new StateChangeWorker(env, this, raw, GST_STATE_PAUSED, timeout);
  Napi::Promise promise = worker->GetPromise().Promise();
  state_worker_started();
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

  GstPipeline *raw = require_pipeline(env);
  if (raw == nullptr) return env.Undefined();

  // Create worker and get its promise
  StateChangeWorker *worker = new StateChangeWorker(env, this, raw, GST_STATE_NULL, timeout);
  Napi::Promise promise = worker->GetPromise().Promise();
  state_worker_started();
  worker->Queue();

  return promise;
}

Napi::Value Pipeline::get_element_by_name(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  GstPipeline *raw = require_pipeline(env);
  if (raw == nullptr) return env.Undefined();

  auto name = info[0].As<Napi::String>().Utf8Value();
  GstElement *e = gst_bin_get_by_name(GST_BIN(raw), name.c_str());

  if (e == nullptr) return info.Env().Null();

  // Use the stored constructors to create the appropriate element
  return Element::CreateFromGstElement(info.Env(), e);
}

Napi::Value Pipeline::playing(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  GstPipeline *raw = require_pipeline(env);
  if (raw == nullptr) return env.Undefined();

  GstState state;
  GstState pending;
  GstStateChangeReturn ret =
    gst_element_get_state(GST_ELEMENT(raw), &state, &pending, 5 * GST_MSECOND);

  // If state change is in progress and we're transitioning to PLAYING, consider it as playing
  bool is_playing =
    (state == GST_STATE_PLAYING) || (ret == GST_STATE_CHANGE_ASYNC && pending == GST_STATE_PLAYING);

  return Napi::Boolean::New(info.Env(), is_playing);
}

Napi::Value Pipeline::query_position(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  GstPipeline *raw = require_pipeline(env);
  if (raw == nullptr) return env.Undefined();

  gint64 pos;
  gst_element_query_position(GST_ELEMENT(raw), GST_FORMAT_TIME, &pos);
  double r = pos == -1 ? -1 : (double)pos / GST_SECOND;
  return Napi::Number::New(info.Env(), r);
}

Napi::Value Pipeline::query_duration(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  GstPipeline *raw = require_pipeline(env);
  if (raw == nullptr) return env.Undefined();

  gint64 dur;
  gst_element_query_duration(GST_ELEMENT(raw), GST_FORMAT_TIME, &dur);
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

  GstPipeline *raw = require_pipeline(env);
  if (raw == nullptr) return env.Undefined();

  // Create worker and get its promise
  BusPopWorker *worker = new BusPopWorker(env, raw, timeout);
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

  GstPipeline *raw = require_pipeline(env);
  if (raw == nullptr) return env.Undefined();

  // Convert seconds to nanoseconds
  GstClockTime position_ns = static_cast<GstClockTime>(position_seconds * GST_SECOND);

  // Perform the seek
  gboolean result = gst_element_seek(
    GST_ELEMENT(raw),
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

Napi::Value Pipeline::end_of_stream(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  GstPipeline *raw = require_pipeline(env);
  if (raw == nullptr) return env.Undefined();

  // Query pipeline state with a 5ms timeout
  // Note: Sending EOS to a PAUSED pipeline where sinks have not yet prerolled
  // may block, as gst_element_send_event delivers through the streaming thread
  // which waits on the preroll condition. This is a GStreamer-level behavior.
  GstState state;
  GstState pending;
  gst_element_get_state(GST_ELEMENT(raw), &state, &pending, 5 * GST_MSECOND);

  // Only send EOS if pipeline is in PLAYING or PAUSED state
  if (state != GST_STATE_PLAYING && state != GST_STATE_PAUSED) {
    return Napi::Boolean::New(env, false);
  }

  // Send EOS event to the pipeline
  gboolean result = gst_element_send_event(GST_ELEMENT(raw), gst_event_new_eos());

  return Napi::Boolean::New(env, result);
}

Napi::Value Pipeline::dispose(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Idempotent: once disposed, further calls are no-ops.
  if (disposed) return env.Undefined();

  // Mark disposed immediately so use-after-dispose is guarded and a second
  // dispose() is a no-op, regardless of whether the native teardown runs now or
  // is deferred below.
  disposed = true;

  // dispose() drives the pipeline to NULL and drops the native reference. It
  // must not do that while a state-change worker (play/pause/stop) is still in
  // flight: that worker holds its own reference and can drive the state back up
  // after we force NULL, leaving it to finalize a non-NULL pipeline — which
  // leaks exactly what dispose() exists to reclaim. So when a worker is in
  // flight, defer the teardown; the last worker to finish runs it (see
  // state_worker_finished()). In-flight busPop()/pull workers do not change
  // state, so they do not need to gate the teardown.
  if (in_flight_state_changes > 0) return env.Undefined();

  release_native_pipeline();

  return env.Undefined();
}

Napi::Value Pipeline::ElementExists(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "elementExists() requires a string argument (element name)")
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  ensure_gst_initialized();

  std::string name = info[0].As<Napi::String>().Utf8Value();

  GstElementFactory *factory = gst_element_factory_find(name.c_str());
  bool exists = (factory != nullptr);

  if (factory) {
    gst_object_unref(factory);
  }

  return Napi::Boolean::New(env, exists);
}
