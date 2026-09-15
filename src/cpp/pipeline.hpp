#pragma once

#include <gst/gst.h>
#include <gst/video/video.h>
#include <memory>
#include <napi.h>
#include <string>

class Pipeline : public Napi::ObjectWrap<Pipeline> {
public:
  static Napi::Object Init(const Napi::Env &env, const Napi::Object &exports);
  static Napi::Value ElementExists(const Napi::CallbackInfo &info);

  Pipeline(const Napi::CallbackInfo &info);

  Napi::Value play(const Napi::CallbackInfo &info);
  Napi::Value pause(const Napi::CallbackInfo &info);
  Napi::Value stop(const Napi::CallbackInfo &info);
  Napi::Value playing(const Napi::CallbackInfo &info);
  Napi::Value get_element_by_name(const Napi::CallbackInfo &info);
  Napi::Value query_position(const Napi::CallbackInfo &info);
  Napi::Value query_duration(const Napi::CallbackInfo &info);
  Napi::Value bus_pop(const Napi::CallbackInfo &info);
  Napi::Value seek(const Napi::CallbackInfo &info);
  Napi::Value end_of_stream(const Napi::CallbackInfo &info);
  Napi::Value dispose(const Napi::CallbackInfo &info);

  void state_worker_started();
  void state_worker_finished();

private:
  std::string pipeline_string;
  std::unique_ptr<GstPipeline, decltype(&gst_object_unref)> pipeline;
  bool disposed;
  int in_flight_state_changes;
  static bool gst_initialized;
  static void ensure_gst_initialized();
  GstPipeline *require_pipeline(const Napi::Env &env);
  void release_native_pipeline();
};
