#pragma once

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <memory>
#include <napi.h>
#include <string>

class Element : public Napi::ObjectWrap<Element> {
public:
  static Napi::Object CreateFromGstElement(Napi::Env env, GstElement *element);

  Element(const Napi::CallbackInfo &info);
  virtual ~Element() = default;

  Napi::Value get_element_property(const Napi::CallbackInfo &info);

  Napi::Value pull(const Napi::CallbackInfo &info);

private:
  std::unique_ptr<GstElement, decltype(&gst_object_unref)> element;
};