#pragma once

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <memory>
#include <napi.h>

// Base class for common element functionality
class ElementBase {
protected:
  std::unique_ptr<GstElement, decltype(&gst_object_unref)> element;

  ElementBase() : element(nullptr, gst_object_unref) {}

  void initializeElement(const Napi::CallbackInfo &info);
};

class Element : public Napi::ObjectWrap<Element>, public ElementBase {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::Object CreateFromGstElement(Napi::Env env, GstElement *element);

  Element(const Napi::CallbackInfo &info);
  virtual ~Element() = default;
};

class AppSinkElement : public Napi::ObjectWrap<AppSinkElement>, public ElementBase {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  AppSinkElement(const Napi::CallbackInfo &info);
  virtual ~AppSinkElement() = default;

  Napi::Value pull(const Napi::CallbackInfo &info);
};

class AppSrcElement : public Napi::ObjectWrap<AppSrcElement>, public ElementBase {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  AppSrcElement(const Napi::CallbackInfo &info);
  virtual ~AppSrcElement() = default;
};