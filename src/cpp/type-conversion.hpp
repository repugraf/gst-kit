#pragma once

#include <gst/gst.h>
#include <napi.h>
#include <string>

namespace TypeConversion {
  /**
   * Convert a JavaScript value to a GValue based on the target GType
   * @param env N-API environment
   * @param js_value The JavaScript value to convert
   * @param target_type The target GType for the conversion
   * @param out_value Pointer to the GValue to initialize and set
   * @return true if conversion was successful, false otherwise
   */
  bool js_to_gvalue(Napi::Env env, const Napi::Value& js_value, GType target_type, GValue* out_value);

  /**
   * Convert a GValue to a JavaScript value
   * @param env N-API environment
   * @param gvalue The GValue to convert
   * @return The converted JavaScript value, or Undefined if conversion failed
   */
  Napi::Value gvalue_to_js(Napi::Env env, const GValue* gvalue);

  /**
   * Get a human-readable error message for type conversion failures
   * @param target_type The GType that conversion failed for
   * @param js_value The JavaScript value that couldn't be converted
   * @return Error message string
   */
  std::string get_conversion_error_message(GType target_type, const Napi::Value& js_value);
} 