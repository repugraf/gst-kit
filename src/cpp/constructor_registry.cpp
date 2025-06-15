#include "constructor_registry.hpp"

const char* ConstructorRegistry::CONSTRUCTOR_KEY = "__gst_kit_constructors";

Napi::Object ConstructorRegistry::GetConstructorStorage(Napi::Env env) {
  Napi::Object global = env.Global();
  if (global.Has(CONSTRUCTOR_KEY)) {
    return global.Get(CONSTRUCTOR_KEY).As<Napi::Object>();
  } else {
    Napi::Object constructors = Napi::Object::New(env);
    global.Set(CONSTRUCTOR_KEY, constructors);
    return constructors;
  }
}

void ConstructorRegistry::RegisterConstructor(Napi::Env env, const std::string& name, Napi::Function constructor) {
  Napi::Object constructors = GetConstructorStorage(env);
  constructors.Set(name, constructor);
}

Napi::Function ConstructorRegistry::GetConstructor(Napi::Env env, const std::string& name) {
  Napi::Object constructors = GetConstructorStorage(env);
  if (constructors.Has(name)) {
    return constructors.Get(name).As<Napi::Function>();
  }
  
  // Throw error if constructor not found
  Napi::TypeError::New(env, "Constructor '" + name + "' not found in registry").ThrowAsJavaScriptException();
  return Napi::Function::New(env, [](const Napi::CallbackInfo&) -> Napi::Value { return Napi::Value(); });
}

bool ConstructorRegistry::HasConstructor(Napi::Env env, const std::string& name) {
  Napi::Object constructors = GetConstructorStorage(env);
  return constructors.Has(name);
} 