#pragma once

#include <napi.h>
#include <gst/gst.h>
#include <uv.h>

#include "GLibHelpers.h"

class Pipeline : public Napi::ObjectWrap<Pipeline> {
	public:
		static Napi::Object Init(Napi::Env env, Napi::Object exports);
		static Napi::Object NewInstance(const Napi::CallbackInfo& info, GstPipeline* pipeline);
		Pipeline(const Napi::CallbackInfo& info);
		~Pipeline();

		void play();
		void pause();
		void stop();
		gboolean seek(gint64 time_nanoseconds, GstSeekFlags flags);
		gint64 queryPosition();
		gint64 queryDuration();
		void sendEOS();
		void forceKeyUnit(GObject* sink, int cnt);

		GObject *findChild( const char *name );
		Napi::Value pollBus();

		void setPad( GObject* elem, const char *attribute, const char *padName );
		GObject *getPad( GObject* elem, const char *padName );

	private:
		static Napi::FunctionReference constructor;

		Napi::Value Play(const Napi::CallbackInfo& info);
		Napi::Value Pause(const Napi::CallbackInfo& info);
		Napi::Value Stop(const Napi::CallbackInfo& info);
		Napi::Value Seek(const Napi::CallbackInfo& info);
		Napi::Value QueryPosition(const Napi::CallbackInfo& info);
		Napi::Value QueryDuration(const Napi::CallbackInfo& info);
		Napi::Value SendEOS(const Napi::CallbackInfo& info);
		Napi::Value ForceKeyUnit(const Napi::CallbackInfo& info);
		Napi::Value FindChild(const Napi::CallbackInfo& info);
		Napi::Value SetPad(const Napi::CallbackInfo& info);
		Napi::Value GetPad(const Napi::CallbackInfo& info);
		Napi::Value PollBus(const Napi::CallbackInfo& info);

		Napi::Value GetAutoFlushBus(const Napi::CallbackInfo& info);
		void SetAutoFlushBus(const Napi::CallbackInfo& info, const Napi::Value& value);
		Napi::Value GetDelay(const Napi::CallbackInfo& info);
		void SetDelay(const Napi::CallbackInfo& info, const Napi::Value& value);
		Napi::Value GetLatency(const Napi::CallbackInfo& info);
		void SetLatency(const Napi::CallbackInfo& info, const Napi::Value& value);

		static void _doPollBus( uv_work_t *req );
		static void _polledBus( uv_work_t *req, int );

		GstPipeline *pipeline;
		GstBus *bus;
		bool auto_flush_bus;
		GstClockTime delay;
		GstClockTime latency;
};
