#pragma once

#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <memory>
#include <napi.h>

// Forward declarations
namespace TypeConversion {
  Napi::Object gst_sample_to_js(const Napi::Env &env, GstSample *sample);
}

// AsyncWorker for bus message popping with timeout
class BusPopWorker : public Napi::AsyncWorker {
public:
  BusPopWorker(Napi::Env env, GstPipeline *pipeline, GstClockTime timeout);
  ~BusPopWorker();

  void Execute() override;
  void OnOK() override;
  void OnError(const Napi::Error &error) override;

  Napi::Promise::Deferred GetPromise();

private:
  void cleanup();
  Napi::Object ConvertMessageToJs(const Napi::Env &env, GstMessage *msg);

  GstPipeline *pipeline;
  GstClockTime timeout;
  GstMessage *message;
  Napi::Promise::Deferred deferred;
};

// AsyncWorker for pulling samples with timeout
class PullSampleWorker : public Napi::AsyncWorker {
public:
  PullSampleWorker(Napi::Env env, GstAppSink *app_sink, guint64 timeout_ms);
  ~PullSampleWorker();

  void Execute() override;
  void OnOK() override;
  void OnError(const Napi::Error &error) override;

  Napi::Promise::Deferred GetPromise();

private:
  void cleanup();

  GstAppSink *app_sink;
  guint64 timeout_ms;
  GstSample *sample;
  Napi::Promise::Deferred deferred;
};

// AsyncWorker for pipeline state changes with timeout
class StateChangeWorker : public Napi::AsyncWorker {
public:
  StateChangeWorker(
    Napi::Env env, GstPipeline *pipeline, GstState target_state, GstClockTime timeout
  );
  ~StateChangeWorker();

  void Execute() override;
  void OnOK() override;
  void OnError(const Napi::Error &error) override;

  Napi::Promise::Deferred GetPromise();

private:
  void cleanup();

  GstPipeline *pipeline;
  GstState target_state;
  GstClockTime timeout;
  GstStateChangeReturn state_change_result;
  GstState final_state;
  Napi::Promise::Deferred deferred;
};
