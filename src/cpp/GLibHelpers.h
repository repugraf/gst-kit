#pragma once

#include <napi.h>
#include <gst/gst.h>

Napi::Object createBuffer(Napi::Env env, char *data, int length);

Napi::Value gstsample_to_napi(Napi::Env env, GstSample *sample);
Napi::Value gstvaluearray_to_napi(Napi::Env env, const GValue *gv);
Napi::Value gvalue_to_napi(Napi::Env env, const GValue *gv);
void napi_to_gvalue(Napi::Env env, const Napi::Value& v, GValue *gv, GParamSpec *spec);

gboolean gst_structure_to_napi_value_iterate(GQuark field_id, const GValue *val, gpointer user_data);
Napi::Object gst_structure_to_napi(Napi::Env env, const GstStructure *struc);

class GLibHelpers {
public:
        static Napi::Value GValueToNapiValue(Napi::Env env, const GValue* value);
        static void NapiValueToGValue(Napi::Env env, const Napi::Value& value, GValue* gvalue);
};
