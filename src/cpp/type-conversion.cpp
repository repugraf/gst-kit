#include "type-conversion.hpp"
#include <gst/gst.h>

namespace TypeConversion {

bool js_to_gvalue(Napi::Env env, const Napi::Value& js_value, GType target_type, GValue* out_value) {
  g_value_init(out_value, target_type);

  switch (target_type) {
    case G_TYPE_STRING: {
      if (!js_value.IsString()) {
        g_value_unset(out_value);
        return false;
      }
      std::string str_value = js_value.As<Napi::String>().Utf8Value();
      g_value_set_string(out_value, str_value.c_str());
      return true;
    }
    case G_TYPE_BOOLEAN: {
      if (!js_value.IsBoolean()) {
        g_value_unset(out_value);
        return false;
      }
      g_value_set_boolean(out_value, js_value.As<Napi::Boolean>().Value());
      return true;
    }
    case G_TYPE_INT: {
      if (!js_value.IsNumber()) {
        g_value_unset(out_value);
        return false;
      }
      g_value_set_int(out_value, js_value.As<Napi::Number>().Int32Value());
      return true;
    }
    case G_TYPE_UINT: {
      if (!js_value.IsNumber()) {
        g_value_unset(out_value);
        return false;
      }
      g_value_set_uint(out_value, js_value.As<Napi::Number>().Uint32Value());
      return true;
    }
    case G_TYPE_FLOAT: {
      if (!js_value.IsNumber()) {
        g_value_unset(out_value);
        return false;
      }
      g_value_set_float(out_value, js_value.As<Napi::Number>().FloatValue());
      return true;
    }
    case G_TYPE_DOUBLE: {
      if (!js_value.IsNumber()) {
        g_value_unset(out_value);
        return false;
      }
      g_value_set_double(out_value, js_value.As<Napi::Number>().DoubleValue());
      return true;
    }
    default: {
      // Handle special GStreamer types
      if (target_type == gst_caps_get_type()) {
        // Handle GstCaps - expect string that can be parsed into caps
        if (!js_value.IsString()) {
          g_value_unset(out_value);
          return false;
        }
        std::string caps_str = js_value.As<Napi::String>().Utf8Value();
        GstCaps *caps = gst_caps_from_string(caps_str.c_str());
        if (!caps) {
          g_value_unset(out_value);
          return false;
        }
        g_value_set_boxed(out_value, caps);
        gst_caps_unref(caps);
        return true;
      } else if (G_TYPE_IS_ENUM(target_type)) {
        // Handle enum types
        if (js_value.IsString()) {
          std::string enum_str = js_value.As<Napi::String>().Utf8Value();
          GEnumClass *enum_class = G_ENUM_CLASS(g_type_class_ref(target_type));
          GEnumValue *enum_value = g_enum_get_value_by_nick(enum_class, enum_str.c_str());
          if (!enum_value) {
            enum_value = g_enum_get_value_by_name(enum_class, enum_str.c_str());
          }
          if (enum_value) {
            g_value_set_enum(out_value, enum_value->value);
            g_type_class_unref(enum_class);
            return true;
          } else {
            g_type_class_unref(enum_class);
            g_value_unset(out_value);
            return false;
          }
        } else if (js_value.IsNumber()) {
          g_value_set_enum(out_value, js_value.As<Napi::Number>().Int32Value());
          return true;
        } else {
          g_value_unset(out_value);
          return false;
        }
      } else {
        // For other types, try to convert from string if possible
        if (js_value.IsString() && g_value_type_transformable(G_TYPE_STRING, target_type)) {
          GValue string_value = G_VALUE_INIT;
          g_value_init(&string_value, G_TYPE_STRING);
          std::string str_value = js_value.As<Napi::String>().Utf8Value();
          g_value_set_string(&string_value, str_value.c_str());
          
          if (g_value_transform(&string_value, out_value)) {
            g_value_unset(&string_value);
            return true;
          } else {
            g_value_unset(&string_value);
            g_value_unset(out_value);
            return false;
          }
        } else {
          g_value_unset(out_value);
          return false;
        }
      }
    }
  }
}

Napi::Value gvalue_to_js(Napi::Env env, const GValue* gvalue) {
  switch (G_VALUE_TYPE(gvalue)) {
    case G_TYPE_STRING:
      return Napi::String::New(env, g_value_get_string(gvalue));
    case G_TYPE_BOOLEAN:
      return Napi::Boolean::New(env, g_value_get_boolean(gvalue));
    case G_TYPE_INT:
      return Napi::Number::New(env, g_value_get_int(gvalue));
    case G_TYPE_UINT:
      return Napi::Number::New(env, g_value_get_uint(gvalue));
    case G_TYPE_FLOAT:
      return Napi::Number::New(env, g_value_get_float(gvalue));
    case G_TYPE_DOUBLE:
      return Napi::Number::New(env, g_value_get_double(gvalue));
    default:
      // Handle enum types first
      if (G_TYPE_IS_ENUM(G_VALUE_TYPE(gvalue))) {
        gint enum_val = g_value_get_enum(gvalue);
        GEnumClass *enum_class = G_ENUM_CLASS(g_type_class_ref(G_VALUE_TYPE(gvalue)));
        GEnumValue *enum_value = g_enum_get_value(enum_class, enum_val);
        if (enum_value && enum_value->value_nick) {
          Napi::Value result = Napi::String::New(env, enum_value->value_nick);
          g_type_class_unref(enum_class);
          return result;
        } else if (enum_value && enum_value->value_name) {
          Napi::Value result = Napi::String::New(env, enum_value->value_name);
          g_type_class_unref(enum_class);
          return result;
        } else {
          g_type_class_unref(enum_class);
          return Napi::Number::New(env, enum_val);
        }
      }
      // Handle GstCaps and other complex types by converting to string
      if (G_VALUE_HOLDS_BOXED(gvalue)) {
        gpointer boxed_value = g_value_get_boxed(gvalue);
        if (boxed_value && GST_IS_CAPS(boxed_value)) {
          GstCaps *caps = GST_CAPS(boxed_value);
          gchar *caps_str = gst_caps_to_string(caps);
          if (caps_str) {
            Napi::Value result = Napi::String::New(env, caps_str);
            g_free(caps_str);
            return result;
          }
        }
      }
      break;
  }
  
  return env.Undefined();
}

std::string get_conversion_error_message(GType target_type, const Napi::Value& js_value) {
  std::string js_type;
  if (js_value.IsString()) js_type = "string";
  else if (js_value.IsNumber()) js_type = "number";
  else if (js_value.IsBoolean()) js_type = "boolean";
  else if (js_value.IsNull()) js_type = "null";
  else if (js_value.IsUndefined()) js_type = "undefined";
  else if (js_value.IsObject()) js_type = "object";
  else js_type = "unknown";

  switch (target_type) {
    case G_TYPE_STRING:
      return "Expected string value, got " + js_type;
    case G_TYPE_BOOLEAN:
      return "Expected boolean value, got " + js_type;
    case G_TYPE_INT:
    case G_TYPE_UINT:
    case G_TYPE_FLOAT:
    case G_TYPE_DOUBLE:
      return "Expected number value, got " + js_type;
    default:
      if (target_type == gst_caps_get_type()) {
        return "Expected string value for caps property, got " + js_type;
      } else if (G_TYPE_IS_ENUM(target_type)) {
        return "Expected string or number value for enum property, got " + js_type;
      } else {
        return "Cannot convert " + js_type + " to " + std::string(g_type_name(target_type));
      }
  }
}

} 