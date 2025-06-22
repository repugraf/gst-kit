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
  bool js_to_gvalue(
    Napi::Env env, const Napi::Value &js_value, GType target_type, GValue *out_value
  );

  /**
   * Convert a GValue to a JavaScript value
   * @param env N-API environment
   * @param gvalue The GValue to convert
   * @return The converted JavaScript value (throws on conversion failure)
   */
  Napi::Value gvalue_to_js(Napi::Env env, const GValue *gvalue);

  /**
   * Convert a GValue to a standardized JavaScript object with type and value fields
   * @param env N-API environment
   * @param gvalue The GValue to convert
   * @return Object with 'type' and 'value' fields, or null if the value is null
   */
  Napi::Value gvalue_to_js_with_type(Napi::Env env, const GValue *gvalue);

  /**
   * Get a human-readable error message for type conversion failures
   * @param target_type The GType that conversion failed for
   * @param js_value The JavaScript value that couldn't be converted
   * @return Error message string
   */
  std::string get_conversion_error_message(GType target_type, const Napi::Value &js_value);

  /**
   * Convert a GstSample to a JavaScript object with buf, caps, and flags properties
   * @param env N-API environment
   * @param sample The GstSample to convert
   * @return JavaScript object with sample data
   */
  Napi::Object gst_sample_to_js(Napi::Env env, GstSample *sample);

  /**
   * Convert a GstStructure to a JavaScript object
   * @param env N-API environment
   * @param structure The GstStructure to convert
   * @return JavaScript object with structure data
   */
  Napi::Object gst_structure_to_js(Napi::Env env, const GstStructure *structure);
}