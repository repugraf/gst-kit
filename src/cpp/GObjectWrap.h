#pragma once

#include <napi.h>
#include <gst/gst.h>

#include "GLibHelpers.h"

class GObjectWrap : public Napi::ObjectWrap<GObjectWrap> {
	public:
		static Napi::Object Init(Napi::Env env, Napi::Object exports);
		static Napi::Object NewInstance(const Napi::CallbackInfo& info, GObject* obj);
		GObjectWrap(const Napi::CallbackInfo& info);
		~GObjectWrap();

		void set(const char *name, const Napi::Value& value);

		void play();
		void pause();
		void stop();

	private:
		static Napi::FunctionReference constructor;

		Napi::Value GetProperty(const Napi::CallbackInfo& info);
		Napi::Value SetProperty(const Napi::CallbackInfo& info);
		Napi::Value Pull(const Napi::CallbackInfo& info);
		Napi::Value GstAppSinkPull(const Napi::CallbackInfo& info);
		Napi::Value GstAppSrcPush(const Napi::CallbackInfo& info);

		GObject* obj;
};
