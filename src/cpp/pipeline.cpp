#include "pipeline.hpp"
#include <gst/gst.h>
#include <gst/video/video.h>

Napi::Object Pipeline::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "Pipeline",
    {InstanceMethod("play", &Pipeline::play), InstanceMethod("stop", &Pipeline::stop),
     InstanceMethod("playing", &Pipeline::playing),
     InstanceMethod("getElementByName", &Pipeline::get_element_by_name)}
  );

  exports.Set("Pipeline", func);
  return exports;
}

Pipeline::Pipeline(const Napi::CallbackInfo &info) :
    Napi::ObjectWrap<Pipeline>(info), _pipeline(nullptr, gst_object_unref) {
  gst_init(NULL, NULL);
  Napi::Env env = info.Env();
  GError *err = NULL;

  if (info.Length() > 0 && info[0].IsString()) {
    _pipeline_string = info[0].As<Napi::String>().Utf8Value();
  } else {
    Napi::Error::New(env, "Wrong type value for pipeline string").ThrowAsJavaScriptException();
  }

  GstPipeline *raw_pipeline =
    (GstPipeline *)GST_BIN(gst_parse_launch(_pipeline_string.c_str(), &err));
  if (err) {
    Napi::Error::New(env, err->message).ThrowAsJavaScriptException();
  }

  _pipeline.reset(raw_pipeline);

  // Set methods as enumerable instance properties to make them visible in console.log
  Napi::Object thisObj = info.This().As<Napi::Object>();

  // Create bound methods
  auto play_method =
    Napi::Function::New(env, [this](const Napi::CallbackInfo &info) -> Napi::Value {
      return this->play(info);
    });
  auto stop_method =
    Napi::Function::New(env, [this](const Napi::CallbackInfo &info) -> Napi::Value {
      return this->stop(info);
    });
  auto playing_method =
    Napi::Function::New(env, [this](const Napi::CallbackInfo &info) -> Napi::Value {
      return this->playing(info);
    });
  auto get_element_by_name_method =
    Napi::Function::New(env, [this](const Napi::CallbackInfo &info) -> Napi::Value {
      return this->get_element_by_name(info);
    });

  thisObj.DefineProperties(
    {Napi::PropertyDescriptor::Value("play", play_method, napi_enumerable),
     Napi::PropertyDescriptor::Value("stop", stop_method, napi_enumerable),
     Napi::PropertyDescriptor::Value("playing", playing_method, napi_enumerable),
     Napi::PropertyDescriptor::Value(
       "getElementByName", get_element_by_name_method, napi_enumerable
     )}
  );
}

Napi::Value Pipeline::play(const Napi::CallbackInfo &info) {
  gst_element_set_state(GST_ELEMENT(_pipeline.get()), GST_STATE_PLAYING);
  return info.Env().Undefined();
}

Napi::Value Pipeline::stop(const Napi::CallbackInfo &info) {
  gst_element_set_state(GST_ELEMENT(_pipeline.get()), GST_STATE_NULL);
  return info.Env().Undefined();
}

Napi::Value Pipeline::get_element_by_name(const Napi::CallbackInfo &info) {
  auto name = info[0].As<Napi::String>().Utf8Value();
  GstElement *e = gst_bin_get_by_name(GST_BIN(_pipeline.get()), name.c_str());
  
  if (e == nullptr) return info.Env().Null();
  
  
  // gst_bin_get_by_name returns a reference, so we don't need to add one
  return Napi::External<GstElement>::New(info.Env(), e, [](Napi::Env env, GstElement* element) {
    gst_object_unref(element);
  });
}

Napi::Value Pipeline::playing(const Napi::CallbackInfo &info) {
  GstState state;
  GstState pending;
  GstStateChangeReturn ret =
    gst_element_get_state(GST_ELEMENT(_pipeline.get()), &state, &pending, 0);

  return Napi::Boolean::New(info.Env(), (state == GST_STATE_PLAYING));
}