#include "Pipeline.h"
#include "GLibHelpers.h"
#include <napi.h>
#include <node_api.h>
#include <uv.h>

#include <gst/gst.h>
#include <gst/video/video.h>

#include "GObjectWrap.h"

Napi::FunctionReference Pipeline::constructor;

#define NANOS_TO_DOUBLE(nanos)(((double)nanos)/1000000000)
//FIXME: guint64 overflow
#define DOUBLE_TO_NANOS(secs)((guint64)(secs*1000000000))

class BusRequest {
public:
	BusRequest(Pipeline* pipeline, Napi::Function cb)
		: pipeline(pipeline) {
		callback = Napi::Reference<Napi::Function>::New(cb, 1);
		request.data = this;
	}

	~BusRequest() {
		callback.Unref();
	}

	Pipeline* pipeline;
	uv_work_t request;
	Napi::Reference<Napi::Function> callback;
};

void Pipeline::_doPollBus(uv_work_t* req) {
	BusRequest* br = static_cast<BusRequest*>(req->data);
	GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(br->pipeline->pipeline));
	GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
			(GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED));
	if (msg) {
		gst_message_unref(msg);
	}
	gst_object_unref(bus);
}

void Pipeline::_polledBus(uv_work_t* req, int status) {
	BusRequest* br = static_cast<BusRequest*>(req->data);
	Napi::Env env = br->callback.Env();
	Napi::HandleScope scope(env);

	br->callback.Value().Call({});
	delete br;
}

Napi::Object Pipeline::Init(Napi::Env env, Napi::Object exports) {
	Napi::Function func = DefineClass(env, "Pipeline", {
		InstanceMethod("play", &Pipeline::Play),
		InstanceMethod("pause", &Pipeline::Pause),
		InstanceMethod("stop", &Pipeline::Stop),
		InstanceMethod("seek", &Pipeline::Seek),
		InstanceMethod("queryPosition", &Pipeline::QueryPosition),
		InstanceMethod("queryDuration", &Pipeline::QueryDuration),
		InstanceMethod("sendEOS", &Pipeline::SendEOS),
		InstanceMethod("forceKeyUnit", &Pipeline::ForceKeyUnit),
		InstanceMethod("findChild", &Pipeline::FindChild),
		InstanceMethod("setPad", &Pipeline::SetPad),
		InstanceMethod("getPad", &Pipeline::GetPad),
		InstanceMethod("pollBus", &Pipeline::PollBus),
		InstanceAccessor("autoFlushBus", &Pipeline::GetAutoFlushBus, &Pipeline::SetAutoFlushBus),
		InstanceAccessor("delay", &Pipeline::GetDelay, &Pipeline::SetDelay),
		InstanceAccessor("latency", &Pipeline::GetLatency, &Pipeline::SetLatency)
	});

	constructor = Napi::Persistent(func);
	constructor.SuppressDestruct();

	exports.Set("Pipeline", func);
	return exports;
}

Pipeline::Pipeline(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Pipeline>(info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);

	if (info.Length() < 1) {
		Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
		return;
	}

	if (!info[0].IsString()) {
		Napi::TypeError::New(env, "First argument must be a string").ThrowAsJavaScriptException();
		return;
	}

	std::string pipeline_str = info[0].As<Napi::String>();
	GError* error = nullptr;
	pipeline = GST_PIPELINE(gst_parse_launch(pipeline_str.c_str(), &error));
	if (error) {
		Napi::Error::New(env, error->message).ThrowAsJavaScriptException();
		g_error_free(error);
		return;
	}

	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	auto_flush_bus = true;
	delay = 0;
	latency = 0;
}

Pipeline::~Pipeline() {
	if (bus) {
		gst_object_unref(bus);
	}
	if (pipeline) {
		gst_object_unref(pipeline);
	}
}

Napi::Object Pipeline::NewInstance(const Napi::CallbackInfo& info, GstPipeline* pipeline) {
	Napi::Env env = info.Env();
	Napi::EscapableHandleScope scope(env);

	Napi::Object obj = constructor.New({});
	Pipeline* pipeline_obj = Napi::ObjectWrap<Pipeline>::Unwrap(obj);
	pipeline_obj->pipeline = pipeline;
	pipeline_obj->bus = gst_pipeline_get_bus(pipeline);
	pipeline_obj->auto_flush_bus = true;
	pipeline_obj->delay = 0;
	pipeline_obj->latency = 0;

	return scope.Escape(obj).As<Napi::Object>();
}

Napi::Value Pipeline::Play(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Pipeline* obj = Napi::ObjectWrap<Pipeline>::Unwrap(info.This().As<Napi::Object>());
	gst_element_set_state(GST_ELEMENT(obj->pipeline), GST_STATE_PLAYING);
	return env.Undefined();
}

Napi::Value Pipeline::Pause(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Pipeline* obj = Napi::ObjectWrap<Pipeline>::Unwrap(info.This().As<Napi::Object>());
	gst_element_set_state(GST_ELEMENT(obj->pipeline), GST_STATE_PAUSED);
	return env.Undefined();
}

Napi::Value Pipeline::Stop(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Pipeline* obj = Napi::ObjectWrap<Pipeline>::Unwrap(info.This().As<Napi::Object>());
	gst_element_set_state(GST_ELEMENT(obj->pipeline), GST_STATE_NULL);
	return env.Undefined();
}

void Pipeline::sendEOS() {
	gst_element_send_event(GST_ELEMENT(pipeline), gst_event_new_eos());
}

Napi::Value Pipeline::SendEOS(const Napi::CallbackInfo& info) {
	sendEOS();
	return info.Env().Undefined();
}

gboolean Pipeline::seek(gint64 time_seconds, GstSeekFlags flags) {
	return gst_element_seek(GST_ELEMENT(pipeline), 1.0, GST_FORMAT_TIME, flags,
						  GST_SEEK_TYPE_SET, time_seconds,
						  GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
}

Napi::Value Pipeline::Seek(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	
	if (info.Length() < 2) {
		Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
		return env.Null();
	}

	gint64 t = info[0].As<Napi::Number>().Int64Value() * GST_SECOND;
	GstSeekFlags flags = (GstSeekFlags)info[1].As<Napi::Number>().Int32Value();

	return Napi::Boolean::New(env, seek(t, flags));
}

gint64 Pipeline::queryPosition() {
	gint64 pos;
	gst_element_query_position(GST_ELEMENT(pipeline), GST_FORMAT_TIME, &pos);
	return pos;
}

Napi::Value Pipeline::QueryPosition(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	gint64 t = queryPosition();
	double r = t == -1 ? -1 : (double)t/GST_SECOND;
	return Napi::Number::New(env, r);
}

gint64 Pipeline::queryDuration() {
	gint64 dur;
	gst_element_query_duration(GST_ELEMENT(pipeline), GST_FORMAT_TIME, &dur);
	return dur;
}

Napi::Value Pipeline::QueryDuration(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	gint64 t = queryDuration();
	double r = t == -1 ? -1 : (double)t/GST_SECOND;
	return Napi::Number::New(env, r);
}

void Pipeline::forceKeyUnit(GObject *sink, int cnt) {
	GstPad *sinkpad = gst_element_get_static_pad(GST_ELEMENT(sink), "sink");
	gst_pad_push_event(sinkpad, (GstEvent*)gst_video_event_new_upstream_force_key_unit(GST_CLOCK_TIME_NONE, TRUE, cnt));
}

Napi::Value Pipeline::ForceKeyUnit(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	
	if (info.Length() < 2) {
		Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
		return env.Null();
	}

	std::string name = info[0].As<Napi::String>().Utf8Value();
	int cnt = info[1].As<Napi::Number>().Int32Value();
	
	GObject *o = findChild(name.c_str());
	if (!o) {
		Napi::Error::New(env, "Element not found").ThrowAsJavaScriptException();
		return env.Null();
	}

	forceKeyUnit(o, cnt);
	return Napi::Boolean::New(env, true);
}

GObject *Pipeline::findChild(const char *name) {
	GstElement *e = gst_bin_get_by_name(GST_BIN(pipeline), name);
	return G_OBJECT(e);
}

Napi::Value Pipeline::FindChild(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	
	if (info.Length() < 1) {
		Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
		return env.Null();
	}

	std::string name = info[0].As<Napi::String>().Utf8Value();
	GObject *o = findChild(name.c_str());
	
	if (o) {
		return GObjectWrap::NewInstance(info, o);
	}
	return env.Null();
}

void Pipeline::setPad(GObject *elem, const char *attribute, const char *padName) {
	GstPad *pad = gst_element_get_static_pad(GST_ELEMENT(elem), padName);
	if (!pad) {
		return;
	}
	g_object_set(elem, attribute, pad, NULL);
}

Napi::Value Pipeline::SetPad(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	
	if (info.Length() < 3) {
		Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
		return env.Null();
	}

	std::string name = info[0].As<Napi::String>().Utf8Value();
	std::string attribute = info[1].As<Napi::String>().Utf8Value();
	std::string padName = info[2].As<Napi::String>().Utf8Value();

	GObject *o = findChild(name.c_str());
	if (!o) {
		Napi::Error::New(env, "Element not found").ThrowAsJavaScriptException();
		return env.Null();
	}

	setPad(o, attribute.c_str(), padName.c_str());
	return env.Undefined();
}

GObject *Pipeline::getPad(GObject* elem, const char *padName ) {
	GstPad *pad = gst_element_get_static_pad(GST_ELEMENT(elem), padName);
	return G_OBJECT(pad);
}

Napi::Value Pipeline::GetPad(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	
	if (info.Length() < 2) {
		Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
		return env.Null();
	}

	std::string name = info[0].As<Napi::String>().Utf8Value();
	GObject *o = findChild(name.c_str());
	if (!o) {
		return env.Null();
	}

	std::string padName = info[1].As<Napi::String>().Utf8Value();
	GObject *pad = getPad(o, padName.c_str());
	if(pad) {
		return GObjectWrap::NewInstance(info, pad);
	} else {
		return env.Null();
	}
}

Napi::Value Pipeline::PollBus(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);

	Pipeline* obj = Napi::ObjectWrap<Pipeline>::Unwrap(info.This().As<Napi::Object>());

	if (info.Length() == 0 || !info[0].IsFunction()) {
		Napi::TypeError::New(env, "Callback function required").ThrowAsJavaScriptException();
		return env.Undefined();
	}

	BusRequest* br = new BusRequest(obj, info[0].As<Napi::Function>());
	uv_queue_work(uv_default_loop(), &br->request, _doPollBus, _polledBus);

	return env.Undefined();
}

Napi::Value Pipeline::GetAutoFlushBus(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Pipeline* obj = Napi::ObjectWrap<Pipeline>::Unwrap(info.This().As<Napi::Object>());
	return Napi::Boolean::New(env, gst_pipeline_get_auto_flush_bus(obj->pipeline));
}

void Pipeline::SetAutoFlushBus(const Napi::CallbackInfo& info, const Napi::Value& value) {
	Napi::Env env = info.Env();
	Pipeline* obj = Napi::ObjectWrap<Pipeline>::Unwrap(info.This().As<Napi::Object>());
	gst_pipeline_set_auto_flush_bus(obj->pipeline, value.As<Napi::Boolean>());
}

Napi::Value Pipeline::GetDelay(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Pipeline* obj = Napi::ObjectWrap<Pipeline>::Unwrap(info.This().As<Napi::Object>());
	GstClockTime delay = gst_pipeline_get_delay(obj->pipeline);
	return Napi::Number::New(env, GST_TIME_AS_SECONDS(delay));
}

void Pipeline::SetDelay(const Napi::CallbackInfo& info, const Napi::Value& value) {
	Napi::Env env = info.Env();
	Pipeline* obj = Napi::ObjectWrap<Pipeline>::Unwrap(info.This().As<Napi::Object>());
	gst_pipeline_set_delay(obj->pipeline, GST_SECOND * value.As<Napi::Number>().DoubleValue());
}

Napi::Value Pipeline::GetLatency(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Pipeline* obj = Napi::ObjectWrap<Pipeline>::Unwrap(info.This().As<Napi::Object>());
	GstClockTime latency = gst_pipeline_get_latency(obj->pipeline);
	return Napi::Number::New(env, GST_TIME_AS_SECONDS(latency));
}

void Pipeline::SetLatency(const Napi::CallbackInfo& info, const Napi::Value& value) {
	Napi::Env env = info.Env();
	Pipeline* obj = Napi::ObjectWrap<Pipeline>::Unwrap(info.This().As<Napi::Object>());
	gst_pipeline_set_latency(obj->pipeline, GST_SECOND * value.As<Napi::Number>().DoubleValue());
}
