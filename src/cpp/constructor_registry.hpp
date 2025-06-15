#pragma once

#include <napi.h>
#include <string>

class ConstructorRegistry {
public:
  // Store a constructor in the global registry
  static void RegisterConstructor(Napi::Env env, const std::string& name, Napi::Function constructor);
  
  // Get a constructor from the global registry
  static Napi::Function GetConstructor(Napi::Env env, const std::string& name);
  
  // Check if a constructor exists in the registry
  static bool HasConstructor(Napi::Env env, const std::string& name);

private:
  static const char* CONSTRUCTOR_KEY;
  static Napi::Object GetConstructorStorage(Napi::Env env);
}; 