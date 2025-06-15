#include "element.hpp"

// Element Implementation
Napi::Object Element::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "Element", {});
  exports.Set("Element", func);
  return exports;
}

Napi::Object Element::CreateWrapper(Napi::Env env, GstElement *element) {
  if (GST_IS_APP_SINK(element)) {
    Napi::Function constructor = AppSinkElement::DefineClass(env, "AppSinkElement", {});
    return constructor.New({Napi::External<GstElement>::New(env, element)});
  } else if (GST_IS_APP_SRC(element)) {
    Napi::Function constructor = AppSrcElement::DefineClass(env, "AppSrcElement", {});
    return constructor.New({Napi::External<GstElement>::New(env, element)});
  } else {
    Napi::Function constructor = Element::DefineClass(env, "Element", {});
    return constructor.New({Napi::External<GstElement>::New(env, element)});
  }
}

Element::Element(const Napi::CallbackInfo &info) :
    Napi::ObjectWrap<Element>(info), element(nullptr, gst_object_unref) {
  if (info.Length() > 0 && info[0].IsExternal()) {
    GstElement *elem = info[0].As<Napi::External<GstElement>>().Data();
    element.reset(elem);
  }
}

// AppSinkElement Implementation
Napi::Object AppSinkElement::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "AppSinkElement", {});
  exports.Set("AppSinkElement", func);
  return exports;
}

AppSinkElement::AppSinkElement(const Napi::CallbackInfo &info) : Element(info) {
}

// AppSrcElement Implementation
Napi::Object AppSrcElement::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "AppSrcElement", {});
  exports.Set("AppSrcElement", func);
  return exports;
}

AppSrcElement::AppSrcElement(const Napi::CallbackInfo &info) : Element(info) {
}