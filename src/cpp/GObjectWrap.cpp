#include <napi.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <string>

#include "GObjectWrap.h"
#include "GLibHelpers.h"

Napi::FunctionReference GObjectWrap::constructor;

Napi::Object GObjectWrap::Init(Napi::Env env, Napi::Object exports) {
	Napi::Function func = DefineClass(env, "GObject", {
		InstanceMethod("getProperty", &GObjectWrap::GetProperty),
		InstanceMethod("setProperty", &GObjectWrap::SetProperty),
		InstanceMethod("pull", &GObjectWrap::Pull),
		InstanceMethod("gstAppSinkPull", &GObjectWrap::GstAppSinkPull),
		InstanceMethod("gstAppSrcPush", &GObjectWrap::GstAppSrcPush)
	});

	constructor = Napi::Persistent(func);
	constructor.SuppressDestruct();

	exports.Set("GObject", func);
	return exports;
}

GObjectWrap::GObjectWrap(const Napi::CallbackInfo& info) : Napi::ObjectWrap<GObjectWrap>(info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);

	if (info.Length() < 1) {
		Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
		return;
	}

	if (!info[0].IsExternal()) {
		Napi::TypeError::New(env, "First argument must be an external GObject").ThrowAsJavaScriptException();
		return;
	}

	obj = static_cast<GObject*>(info[0].As<Napi::External<void>>().Data());
	g_object_ref(obj);

	// Create a JavaScript object to hold the properties
	Napi::Object properties = Napi::Object::New(env);

	// Get the element name
	const gchar* name = gst_object_get_name(GST_OBJECT(obj));
	if (name) {
		properties.Set("name", Napi::String::New(env, name));
	}

	// Get parent if exists
	GstObject* parent = gst_object_get_parent(GST_OBJECT(obj));
	if (parent) {
		properties.Set("parent", Napi::String::New(env, gst_object_get_name(parent)));
		gst_object_unref(parent);
	}

	// If this is a GstAppSink, add its specific properties
	if (GST_IS_APP_SINK(obj)) {
		GstAppSink* appsink = GST_APP_SINK(obj);
		
		// Get sync property
		gboolean sync;
		g_object_get(G_OBJECT(appsink), "sync", &sync, NULL);
		properties.Set("sync", Napi::Boolean::New(env, sync));

		// Get max-lateness
		gint64 max_lateness;
		g_object_get(G_OBJECT(appsink), "max-lateness", &max_lateness, NULL);
		properties.Set("max-lateness", Napi::String::New(env, std::to_string(max_lateness)));

		// Get qos property
		gboolean qos;
		g_object_get(G_OBJECT(appsink), "qos", &qos, NULL);
		properties.Set("qos", Napi::Boolean::New(env, qos));

		// Get async property
		gboolean async;
		g_object_get(G_OBJECT(appsink), "async", &async, NULL);
		properties.Set("async", Napi::Boolean::New(env, async));

		// Get ts-offset
		gint64 ts_offset;
		g_object_get(G_OBJECT(appsink), "ts-offset", &ts_offset, NULL);
		properties.Set("ts-offset", Napi::String::New(env, std::to_string(ts_offset)));

		// Get enable-last-sample
		gboolean enable_last_sample;
		g_object_get(G_OBJECT(appsink), "enable-last-sample", &enable_last_sample, NULL);
		properties.Set("enable-last-sample", Napi::Boolean::New(env, enable_last_sample));

		// Get last-sample
		GstSample* last_sample = gst_app_sink_try_pull_sample(appsink, 0);
		if (last_sample) {
			Napi::Object sample_obj = Napi::Object::New(env);
			GstBuffer* buffer = gst_sample_get_buffer(last_sample);
			if (buffer) {
				GstMapInfo map;
				if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
					sample_obj.Set("buf", Napi::Buffer<uint8_t>::Copy(env, map.data, map.size));
					gst_buffer_unmap(buffer, &map);
				}
			}
			GstCaps* caps = gst_sample_get_caps(last_sample);
			if (caps) {
				sample_obj.Set("caps", Napi::String::New(env, gst_caps_to_string(caps)));
			}
			properties.Set("last-sample", sample_obj);
			gst_sample_unref(last_sample);
		} else {
			// If no sample is available, set last-sample to null
			Napi::Object sample_obj = Napi::Object::New(env);
			sample_obj.Set("buf", env.Null());
			sample_obj.Set("caps", Napi::Object::New(env));
			properties.Set("last-sample", sample_obj);
		}

		// Get blocksize
		guint blocksize;
		g_object_get(G_OBJECT(appsink), "blocksize", &blocksize, NULL);
		properties.Set("blocksize", Napi::Number::New(env, blocksize));

		// Get render-delay
		gint64 render_delay;
		g_object_get(G_OBJECT(appsink), "render-delay", &render_delay, NULL);
		properties.Set("render-delay", Napi::String::New(env, std::to_string(render_delay)));

		// Get throttle-time
		gint64 throttle_time;
		g_object_get(G_OBJECT(appsink), "throttle-time", &throttle_time, NULL);
		properties.Set("throttle-time", Napi::String::New(env, std::to_string(throttle_time)));

		// Get max-bitrate
		guint max_bitrate;
		g_object_get(G_OBJECT(appsink), "max-bitrate", &max_bitrate, NULL);
		properties.Set("max-bitrate", Napi::String::New(env, std::to_string(max_bitrate)));

		// Get processing-deadline
		gint64 processing_deadline;
		g_object_get(G_OBJECT(appsink), "processing-deadline", &processing_deadline, NULL);
		properties.Set("processing-deadline", Napi::String::New(env, std::to_string(processing_deadline)));

		// Get stats
		GstStructure* stats_struct = nullptr;
		g_object_get(G_OBJECT(appsink), "stats", &stats_struct, NULL);
		if (stats_struct) {
			gchar* stats_str = gst_structure_to_string(stats_struct);
			properties.Set("stats", Napi::String::New(env, stats_str));
			g_free(stats_str);
			gst_structure_free(stats_struct);
		} else {
			// Set default stats format if none available
			properties.Set("stats", Napi::String::New(env, "application/x-gst-base-sink-stats, average-rate=(double)0, dropped=(guint64)0, rendered=(guint64)0;"));
		}

		// Get caps
		GstCaps* caps;
		g_object_get(G_OBJECT(appsink), "caps", &caps, NULL);
		if (caps) {
			properties.Set("caps", Napi::String::New(env, gst_caps_to_string(caps)));
			gst_caps_unref(caps);
		} else {
			properties.Set("caps", Napi::String::New(env, "NULL"));
		}

		// Get eos
		gboolean eos;
		g_object_get(G_OBJECT(appsink), "eos", &eos, NULL);
		properties.Set("eos", Napi::Boolean::New(env, eos));

		// Get emit-signals
		gboolean emit_signals;
		g_object_get(G_OBJECT(appsink), "emit-signals", &emit_signals, NULL);
		properties.Set("emit-signals", Napi::Boolean::New(env, emit_signals));

		// Get max-buffers
		guint max_buffers;
		g_object_get(G_OBJECT(appsink), "max-buffers", &max_buffers, NULL);
		properties.Set("max-buffers", Napi::Number::New(env, max_buffers));

		// Get drop
		gboolean drop;
		g_object_get(G_OBJECT(appsink), "drop", &drop, NULL);
		properties.Set("drop", Napi::Boolean::New(env, drop));

		// Get wait-on-eos
		gboolean wait_on_eos;
		g_object_get(G_OBJECT(appsink), "wait-on-eos", &wait_on_eos, NULL);
		properties.Set("wait-on-eos", Napi::Boolean::New(env, wait_on_eos));

		// Get buffer-list
		gboolean buffer_list;
		g_object_get(G_OBJECT(appsink), "buffer-list", &buffer_list, NULL);
		properties.Set("buffer-list", Napi::Boolean::New(env, buffer_list));
	}

	// Set all properties on the JavaScript object
	Napi::Object this_obj = info.This().As<Napi::Object>();
	Napi::Array props = properties.GetPropertyNames();
	for (uint32_t i = 0; i < props.Length(); i++) {
		Napi::String key = props.Get(i).As<Napi::String>();
		Napi::Value val = properties.Get(key);
		this_obj.Set(key, val);
	}

	// Add pull function
	Napi::Function pull_func = Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
		Napi::Env env = info.Env();
		GObjectWrap* wrap = Napi::ObjectWrap<GObjectWrap>::Unwrap(info.This().As<Napi::Object>());
		
		if (!GST_IS_APP_SINK(wrap->obj)) {
			Napi::Error::New(env, "Object is not a GstAppSink").ThrowAsJavaScriptException();
			return env.Undefined();
		}

		if (info.Length() < 1 || !info[0].IsFunction()) {
			Napi::TypeError::New(env, "Callback function required").ThrowAsJavaScriptException();
			return env.Undefined();
		}

		Napi::Function callback = info[0].As<Napi::Function>();
		GstAppSink* appsink = GST_APP_SINK(wrap->obj);
		GstSample* sample = gst_app_sink_pull_sample(appsink);
		
		if (!sample) {
			callback.Call({env.Null()});
			return env.Undefined();
		}

		GstBuffer* buffer = gst_sample_get_buffer(sample);
		GstMapInfo map;
		gst_buffer_map(buffer, &map, GST_MAP_READ);

		Napi::Object result = Napi::Object::New(env);
		result.Set("data", Napi::Buffer<uint8_t>::Copy(env, map.data, map.size));
		result.Set("pts", Napi::Number::New(env, GST_TIME_AS_SECONDS(buffer->pts)));
		result.Set("dts", Napi::Number::New(env, GST_TIME_AS_SECONDS(buffer->dts)));
		result.Set("duration", Napi::Number::New(env, GST_TIME_AS_SECONDS(buffer->duration)));

		gst_buffer_unmap(buffer, &map);
		gst_sample_unref(sample);

		callback.Call({result});
		return env.Undefined();
	}, "pull");
	this_obj.Set("pull", pull_func);
}

GObjectWrap::~GObjectWrap() {
	if (obj) {
		g_object_unref(obj);
	}
}

Napi::Object GObjectWrap::NewInstance(const Napi::CallbackInfo& info, GObject* obj) {
	Napi::Env env = info.Env();
	Napi::EscapableHandleScope scope(env);

	Napi::Object obj_wrap = constructor.New({Napi::External<void>::New(env, obj)});
	return scope.Escape(obj_wrap).As<Napi::Object>();
}

Napi::Value GObjectWrap::GetProperty(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);

	if (info.Length() < 1) {
		Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if (!info[0].IsString()) {
		Napi::TypeError::New(env, "First argument must be a string").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	std::string prop_name = info[0].As<Napi::String>();
	GParamSpec* pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(obj), prop_name.c_str());
	if (!pspec) {
		Napi::Error::New(env, "Property not found").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	GValue value = G_VALUE_INIT;
	g_value_init(&value, pspec->value_type);
	g_object_get_property(obj, prop_name.c_str(), &value);
	Napi::Value result = GLibHelpers::GValueToNapiValue(env, &value);
	g_value_unset(&value);

	return result;
}

Napi::Value GObjectWrap::SetProperty(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);

	if (info.Length() < 2) {
		Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if (!info[0].IsString()) {
		Napi::TypeError::New(env, "First argument must be a string").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	std::string prop_name = info[0].As<Napi::String>();
	GParamSpec* pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(obj), prop_name.c_str());
	if (!pspec) {
		Napi::Error::New(env, "Property not found").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	GValue value = G_VALUE_INIT;
	g_value_init(&value, pspec->value_type);
	GLibHelpers::NapiValueToGValue(env, info[1], &value);
	g_object_set_property(obj, prop_name.c_str(), &value);
	g_value_unset(&value);

	return env.Undefined();
}

Napi::Value GObjectWrap::Pull(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);

	if (!GST_IS_APP_SINK(obj)) {
		Napi::Error::New(env, "Object is not a GstAppSink").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if (info.Length() < 1 || !info[0].IsFunction()) {
		Napi::TypeError::New(env, "Callback function required").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	Napi::Function callback = info[0].As<Napi::Function>();

	GstAppSink* appsink = GST_APP_SINK(obj);
	GstSample* sample = gst_app_sink_pull_sample(appsink);
	if (!sample) {
		callback.Call({env.Null()});
		return env.Undefined();
	}

	GstBuffer* buffer = gst_sample_get_buffer(sample);
	GstMapInfo map;
	gst_buffer_map(buffer, &map, GST_MAP_READ);

	Napi::Object result = Napi::Object::New(env);
	result.Set("data", Napi::Buffer<uint8_t>::Copy(env, map.data, map.size));
	result.Set("pts", Napi::Number::New(env, GST_TIME_AS_SECONDS(buffer->pts)));
	result.Set("dts", Napi::Number::New(env, GST_TIME_AS_SECONDS(buffer->dts)));
	result.Set("duration", Napi::Number::New(env, GST_TIME_AS_SECONDS(buffer->duration)));

	gst_buffer_unmap(buffer, &map);
	gst_sample_unref(sample);

	callback.Call({result});
	return env.Undefined();
}

Napi::Value GObjectWrap::GstAppSinkPull(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);

	GstAppSink* appsink = GST_APP_SINK(obj);
	GstSample* sample = gst_app_sink_pull_sample(appsink);
	if (!sample) {
		return env.Null();
	}

	GstBuffer* buffer = gst_sample_get_buffer(sample);
	GstMapInfo map;
	gst_buffer_map(buffer, &map, GST_MAP_READ);

	Napi::Object result = Napi::Object::New(env);
	result.Set("data", Napi::Buffer<uint8_t>::Copy(env, map.data, map.size));
	result.Set("pts", Napi::Number::New(env, GST_TIME_AS_SECONDS(buffer->pts)));
	result.Set("dts", Napi::Number::New(env, GST_TIME_AS_SECONDS(buffer->dts)));
	result.Set("duration", Napi::Number::New(env, GST_TIME_AS_SECONDS(buffer->duration)));

	gst_buffer_unmap(buffer, &map);
	gst_sample_unref(sample);

	return result;
}

Napi::Value GObjectWrap::GstAppSrcPush(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);

	if (info.Length() < 1) {
		Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	if (!info[0].IsBuffer()) {
		Napi::TypeError::New(env, "First argument must be a buffer").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	Napi::Buffer<uint8_t> buffer = info[0].As<Napi::Buffer<uint8_t>>();
	GstAppSrc* appsrc = GST_APP_SRC(obj);
	GstBuffer* gst_buffer = gst_buffer_new_wrapped_full(
		GST_MEMORY_FLAG_READONLY,
		buffer.Data(),
		buffer.Length(),
		0,
		buffer.Length(),
		nullptr,
		nullptr
	);

	GstFlowReturn ret = gst_app_src_push_buffer(appsrc, gst_buffer);
	if (ret != GST_FLOW_OK) {
		Napi::Error::New(env, "Failed to push buffer").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	return env.Undefined();
}
