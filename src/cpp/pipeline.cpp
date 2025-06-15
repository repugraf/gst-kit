#include "pipeline.hpp"
#include "element.hpp"
#include "constructor_registry.hpp"
#include <gst/gst.h>
#include <gst/video/video.h>

Napi::Object Pipeline::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "Pipeline", {});
  exports.Set("Pipeline", func);
  ConstructorRegistry::RegisterConstructor(env, "Pipeline", func);
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
  gst_element_set_state(GST_ELEMENT(pipeline.get()), GST_STATE_PLAYING);
  return info.Env().Undefined();
}

Napi::Value Pipeline::stop(const Napi::CallbackInfo &info) {
  gst_element_set_state(GST_ELEMENT(pipeline.get()), GST_STATE_NULL);
  return info.Env().Undefined();
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
    gst_element_get_state(GST_ELEMENT(pipeline.get()), &state, &pending, 0);

  return Napi::Boolean::New(info.Env(), (state == GST_STATE_PLAYING));
}