#include <napi.h>
#include <gst/gst.h>
#include <gst/gstcaps.h>

#include "GLibHelpers.h"

Napi::Object createBuffer(Napi::Env env, char *data, int length) {
	return Napi::Buffer<char>::Copy(env, data, length);
}

Napi::Value gstbuffer_to_napi(Napi::Env env, GstBuffer *buf) {
	if(!buf) return env.Null();
	GstMapInfo map;
	if(gst_buffer_map(buf, &map, GST_MAP_READ)) {
		const unsigned char *data = map.data;
		int length = map.size;
		Napi::Object frame = createBuffer(env, (char *)data, length);
		gst_buffer_unmap(buf, &map);
		return frame;
	}
	return env.Undefined();
}

Napi::Value gstsample_to_napi(Napi::Env env, GstSample *sample) {
	if(!sample) return env.Null();
	return gstbuffer_to_napi(env, gst_sample_get_buffer(sample));
}

Napi::Value gstvaluearray_to_napi(Napi::Env env, const GValue *gv) {
	if(!GST_VALUE_HOLDS_ARRAY(gv)) {
		Napi::TypeError::New(env, "not a GstValueArray").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	int size = gst_value_array_get_size(gv);
	Napi::Array array = Napi::Array::New(env, size);

	for(int i = 0; i < size; i++) {
		array.Set(i, gvalue_to_napi(env, gst_value_array_get_value(gv, i)));
	}
	return array;
}

Napi::Value gchararray_to_napi(Napi::Env env, const GValue *gv) {
	const char *str = g_value_get_string(gv);
	if(!str)
		return env.Null();
	return Napi::String::New(env, str);
}

Napi::Value gvalue_to_napi(Napi::Env env, const GValue *gv) {
	switch(G_VALUE_TYPE(gv)) {
		case G_TYPE_STRING:
			return gchararray_to_napi(env, gv);
		case G_TYPE_BOOLEAN:
			return Napi::Boolean::New(env, g_value_get_boolean(gv));
		case G_TYPE_INT:
			return Napi::Number::New(env, g_value_get_int(gv));
		case G_TYPE_UINT:
			return Napi::Number::New(env, g_value_get_uint(gv));
		case G_TYPE_FLOAT:
			return Napi::Number::New(env, g_value_get_float(gv));
		case G_TYPE_DOUBLE:
			return Napi::Number::New(env, g_value_get_double(gv));
	}

	if(GST_VALUE_HOLDS_ARRAY(gv)) {
		return gstvaluearray_to_napi(env, gv);
	} else if(GST_VALUE_HOLDS_BUFFER(gv)) {
		GstBuffer *buf = gst_value_get_buffer(gv);
		return gstbuffer_to_napi(env, buf);
	} else if(GST_VALUE_HOLDS_SAMPLE(gv)) {
		GstSample *sample = gst_value_get_sample(gv);
		Napi::Object caps = Napi::Object::New(env);
		GstCaps *gcaps = gst_sample_get_caps(sample);
		if (gcaps) {
			const GstStructure *structure = gst_caps_get_structure(gcaps, 0);
			if (structure) {
				caps = gst_structure_to_napi(env, structure);
			}
		}

		Napi::Object result = Napi::Object::New(env);
		result.Set("buf", gstsample_to_napi(env, sample));
		result.Set("caps", caps);
		return result;
	}

	/* Attempt to transform it into a GValue of type STRING */
	if(g_value_type_transformable(G_VALUE_TYPE(gv), G_TYPE_STRING)) {
		GValue b = G_VALUE_INIT;
		g_value_init(&b, G_TYPE_STRING);
		g_value_transform(gv, &b);
		return gchararray_to_napi(env, &b);
	}

	return env.Undefined();
}

void napi_to_gvalue(Napi::Env env, const Napi::Value& v, GValue *gv, GParamSpec *spec) {
	if(v.IsNumber()) {
		g_value_init(gv, G_TYPE_FLOAT);
		g_value_set_float(gv, v.As<Napi::Number>().FloatValue());
	} else if(v.IsString()) {
		std::string value = v.As<Napi::String>().Utf8Value();
		if(spec->value_type == GST_TYPE_CAPS) {
			GstCaps* caps = gst_caps_from_string(value.c_str());
			g_value_init(gv, GST_TYPE_CAPS);
			g_value_set_boxed(gv, caps);
		} else {
			g_value_init(gv, G_TYPE_STRING);
			g_value_set_string(gv, value.c_str());
		}
	} else if(v.IsBoolean()) {
		g_value_init(gv, G_TYPE_BOOLEAN);
		g_value_set_boolean(gv, v.As<Napi::Boolean>().Value());
	}
}

/* --------------------------------------------------
    GstStructure helpers
   -------------------------------------------------- */
gboolean gst_structure_to_napi_value_iterate(GQuark field_id, const GValue *val, gpointer user_data) {
	Napi::Object *obj = (Napi::Object*)user_data;
	Napi::Value v = gvalue_to_napi(obj->Env(), val);
	Napi::String name = Napi::String::New(obj->Env(), (const char *)g_quark_to_string(field_id));
	obj->Set(name, v);
	return true;
}

Napi::Object gst_structure_to_napi(Napi::Env env, const GstStructure *struc) {
	Napi::Object obj = Napi::Object::New(env);
	const gchar *name = gst_structure_get_name(struc);
	obj.Set("name", Napi::String::New(env, name));
	gst_structure_foreach(struc, gst_structure_to_napi_value_iterate, &obj);
	return obj;
}

Napi::Value GLibHelpers::GValueToNapiValue(Napi::Env env, const GValue* value) {
	if (!G_IS_VALUE(value)) {
		return env.Null();
	}

	GType type = G_VALUE_TYPE(value);
	if (type == G_TYPE_STRING) {
		return Napi::String::New(env, g_value_get_string(value));
	} else if (type == G_TYPE_INT) {
		return Napi::Number::New(env, g_value_get_int(value));
	} else if (type == G_TYPE_UINT) {
		return Napi::Number::New(env, g_value_get_uint(value));
	} else if (type == G_TYPE_INT64) {
		return Napi::Number::New(env, g_value_get_int64(value));
	} else if (type == G_TYPE_UINT64) {
		return Napi::Number::New(env, g_value_get_uint64(value));
	} else if (type == G_TYPE_FLOAT) {
		return Napi::Number::New(env, g_value_get_float(value));
	} else if (type == G_TYPE_DOUBLE) {
		return Napi::Number::New(env, g_value_get_double(value));
	} else if (type == G_TYPE_BOOLEAN) {
		return Napi::Boolean::New(env, g_value_get_boolean(value));
	} else if (GST_VALUE_HOLDS_FRACTION(value)) {
		gint num = gst_value_get_fraction_numerator(value);
		gint denom = gst_value_get_fraction_denominator(value);
		return Napi::Number::New(env, static_cast<double>(num) / denom);
	} else if (GST_VALUE_HOLDS_INT_RANGE(value)) {
		gint min = gst_value_get_int_range_min(value);
		gint max = gst_value_get_int_range_max(value);
		Napi::Object range = Napi::Object::New(env);
		range.Set("min", Napi::Number::New(env, min));
		range.Set("max", Napi::Number::New(env, max));
		return range;
	} else if (GST_VALUE_HOLDS_DOUBLE_RANGE(value)) {
		gdouble min = gst_value_get_double_range_min(value);
		gdouble max = gst_value_get_double_range_max(value);
		Napi::Object range = Napi::Object::New(env);
		range.Set("min", Napi::Number::New(env, min));
		range.Set("max", Napi::Number::New(env, max));
		return range;
	} else if (GST_VALUE_HOLDS_LIST(value)) {
		guint size = gst_value_list_get_size(value);
		Napi::Array array = Napi::Array::New(env, size);
		for (guint i = 0; i < size; i++) {
			const GValue* item = gst_value_list_get_value(value, i);
			array.Set(i, GValueToNapiValue(env, item));
		}
		return array;
	} else if (GST_VALUE_HOLDS_ARRAY(value)) {
		guint size = gst_value_array_get_size(value);
		Napi::Array array = Napi::Array::New(env, size);
		for (guint i = 0; i < size; i++) {
			const GValue* item = gst_value_array_get_value(value, i);
			array.Set(i, GValueToNapiValue(env, item));
		}
		return array;
	} else if (GST_VALUE_HOLDS_STRUCTURE(value)) {
		const GstStructure* structure = gst_value_get_structure(value);
		Napi::Object obj = Napi::Object::New(env);
		gst_structure_foreach(structure, [](GQuark field_id, const GValue* value, gpointer user_data) -> gboolean {
			Napi::Object* obj = static_cast<Napi::Object*>(user_data);
			const gchar* field_name = g_quark_to_string(field_id);
			obj->Set(field_name, GValueToNapiValue(obj->Env(), value));
			return TRUE;
		}, &obj);
		return obj;
	}

	return env.Null();
}

void GLibHelpers::NapiValueToGValue(Napi::Env env, const Napi::Value& value, GValue* gvalue) {
	if (value.IsString()) {
		g_value_init(gvalue, G_TYPE_STRING);
		g_value_set_string(gvalue, value.As<Napi::String>().Utf8Value().c_str());
	} else if (value.IsNumber()) {
		g_value_init(gvalue, G_TYPE_DOUBLE);
		g_value_set_double(gvalue, value.As<Napi::Number>().DoubleValue());
	} else if (value.IsBoolean()) {
		g_value_init(gvalue, G_TYPE_BOOLEAN);
		g_value_set_boolean(gvalue, value.As<Napi::Boolean>().Value());
	} else if (value.IsArray()) {
		Napi::Array array = value.As<Napi::Array>();
		guint size = array.Length();
		g_value_init(gvalue, GST_TYPE_ARRAY);
		for (guint i = 0; i < size; i++) {
			GValue item = G_VALUE_INIT;
			NapiValueToGValue(env, array.Get(i), &item);
			gst_value_array_append_value(gvalue, &item);
			g_value_unset(&item);
		}
	} else if (value.IsObject()) {
		Napi::Object obj = value.As<Napi::Object>();
		GstStructure* structure = gst_structure_new_empty("object");
		Napi::Array props = obj.GetPropertyNames();
		for (uint32_t i = 0; i < props.Length(); i++) {
			Napi::String key = props.Get(i).As<Napi::String>();
			Napi::Value val = obj.Get(key);
			GValue gval = G_VALUE_INIT;
			NapiValueToGValue(env, val, &gval);
			gst_structure_set_value(structure, key.Utf8Value().c_str(), &gval);
			g_value_unset(&gval);
		}
		g_value_init(gvalue, GST_TYPE_STRUCTURE);
		gst_value_set_structure(gvalue, structure);
		gst_structure_free(structure);
	} else {
		g_value_init(gvalue, G_TYPE_NONE);
	}
}
