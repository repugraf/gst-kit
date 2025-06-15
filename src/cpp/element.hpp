#pragma once

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <memory>
#include <napi.h>

class Element : public Napi::ObjectWrap<Element> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::Object CreateWrapper(Napi::Env env, GstElement *element);

  Element(const Napi::CallbackInfo &info);
  virtual ~Element() = default;

protected:
  std::unique_ptr<GstElement, decltype(&gst_object_unref)> element;
};

class AppSinkElement : public Element {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  AppSinkElement(const Napi::CallbackInfo &info);
  virtual ~AppSinkElement() = default;
};

class AppSrcElement : public Element {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  AppSrcElement(const Napi::CallbackInfo &info);
  virtual ~AppSrcElement() = default;
};