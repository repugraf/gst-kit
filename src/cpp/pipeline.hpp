#pragma once

#include <gst/gst.h>
#include <gst/video/video.h>
#include <memory>
#include <napi.h>
#include <string>

class Pipeline : public Napi::ObjectWrap<Pipeline> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  Pipeline(const Napi::CallbackInfo &info);

  Napi::Value play(const Napi::CallbackInfo &info);
  Napi::Value stop(const Napi::CallbackInfo &info);
  Napi::Value playing(const Napi::CallbackInfo &info);
  Napi::Value get_element_by_name(const Napi::CallbackInfo &info);
  Napi::Value queryPosition(const Napi::CallbackInfo &info);
  Napi::Value queryDuration(const Napi::CallbackInfo &info);

private:
  std::string pipeline_string;
  std::unique_ptr<GstPipeline, decltype(&gst_object_unref)> pipeline;
};