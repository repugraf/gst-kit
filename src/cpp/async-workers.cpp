#include "async-workers.hpp"
#include "type-conversion.hpp"
#include <gst/gst.h>

// BusPopWorker implementation
BusPopWorker::BusPopWorker(Napi::Env env, GstPipeline *pipeline, GstClockTime timeout) :
    Napi::AsyncWorker(env), pipeline(pipeline), timeout(timeout), message(nullptr),
    deferred(env) {
  // Increase reference count since we'll be using this in another thread
  gst_object_ref(pipeline);
}

BusPopWorker::~BusPopWorker() { 
  cleanup(); 
}

void BusPopWorker::Execute() {
  GstBus *bus = gst_element_get_bus(GST_ELEMENT(pipeline));
  if (!bus) {
    return;
  }

  // Use gst_bus_timed_pop to wait for a message with timeout
  message = gst_bus_timed_pop(bus, timeout);
  
  gst_object_unref(bus);
}

Napi::Promise::Deferred BusPopWorker::GetPromise() { 
  return deferred; 
}

void BusPopWorker::OnOK() {
  Napi::HandleScope scope(Env());

  if (message) {
    Napi::Object result = ConvertMessageToJs(Env(), message);
    deferred.Resolve(result);
    return;
  }

  // Timeout or no message
  deferred.Resolve(Env().Null());
}

void BusPopWorker::OnError(const Napi::Error &error) {
  Napi::HandleScope scope(Env());
  deferred.Reject(error.Value());
}

void BusPopWorker::cleanup() {
  if (message) {
    gst_message_unref(message);
    message = nullptr;
  }
  if (pipeline) {
    gst_object_unref(pipeline);
    pipeline = nullptr;
  }
}

Napi::Object BusPopWorker::ConvertMessageToJs(Napi::Env env, GstMessage *msg) {
  Napi::Object result = Napi::Object::New(env);
  
  // Add message type
  result.Set("type", Napi::String::New(env, GST_MESSAGE_TYPE_NAME(msg)));
  
  // Add source element name
  if (msg->src) {
    result.Set("srcElementName", Napi::String::New(env, GST_OBJECT_NAME(msg->src)));
  }
  
  result.Set("timestamp", Napi::BigInt::New(env, msg->timestamp));
  
  // Add structure data if available
  const GstStructure *structure = gst_message_get_structure(msg);
  if (structure) {
    result.Set("structureName", Napi::String::New(env, gst_structure_get_name(structure)));
    
    // Convert structure fields to JavaScript object
    auto callback_data = std::make_pair(env, &result);
    gst_structure_foreach(
      structure,
      [](GQuark field_id, const GValue *value, gpointer user_data) -> gboolean {
        auto *data = static_cast<std::pair<Napi::Env, Napi::Object *> *>(user_data);
        Napi::Env env = data->first;
        Napi::Object *obj = data->second;

        const char *field_name = g_quark_to_string(field_id);
        try {
          Napi::Value js_value = TypeConversion::gvalue_to_js(env, value);
          obj->Set(field_name, js_value);
        } catch (...) {
          // Skip fields that can't be converted
        }

        return TRUE;
      },
      &callback_data
    );
  }
  
  // Add special handling for common message types
  GstMessageType msg_type = GST_MESSAGE_TYPE(msg);
  if (msg_type == GST_MESSAGE_ERROR) {
    GError *err = nullptr;
    gchar *debug = nullptr;
    gst_message_parse_error(msg, &err, &debug);
    
    if (err) {
      result.Set("errorMessage", Napi::String::New(env, err->message));
      result.Set("errorDomain", Napi::String::New(env, g_quark_to_string(err->domain)));
      result.Set("errorCode", Napi::Number::New(env, err->code));
      g_error_free(err);
    }
    
    if (debug) {
      result.Set("debugInfo", Napi::String::New(env, debug));
      g_free(debug);
    }
  } else if (msg_type == GST_MESSAGE_WARNING) {
    GError *err = nullptr;
    gchar *debug = nullptr;
    gst_message_parse_warning(msg, &err, &debug);
    
    if (err) {
      result.Set("warningMessage", Napi::String::New(env, err->message));
      result.Set("warningDomain", Napi::String::New(env, g_quark_to_string(err->domain)));
      result.Set("warningCode", Napi::Number::New(env, err->code));
      g_error_free(err);
    }
    
    if (debug) {
      result.Set("debugInfo", Napi::String::New(env, debug));
      g_free(debug);
    }
  } else if (msg_type == GST_MESSAGE_STATE_CHANGED) {
    GstState old_state, new_state, pending;
    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending);
    
    result.Set("old_state", Napi::Number::New(env, old_state));
    result.Set("new_state", Napi::Number::New(env, new_state));
    result.Set("pendingState", Napi::Number::New(env, pending));
  }
  
  return result;
}

// PullSampleWorker implementation
PullSampleWorker::PullSampleWorker(Napi::Env env, GstAppSink *app_sink, guint64 timeout_ms) :
    Napi::AsyncWorker(env), app_sink(app_sink), timeout_ms(timeout_ms), sample(nullptr),
    deferred(env) {
  // Increase reference count since we'll be using this in another thread
  gst_object_ref(app_sink);
}

PullSampleWorker::~PullSampleWorker() { 
  cleanup(); 
}

void PullSampleWorker::Execute() {
  // Convert timeout from milliseconds to nanoseconds (GstClockTime)
  GstClockTime timeout = timeout_ms * GST_MSECOND;

  // Use GStreamer's built-in timeout mechanism
  sample = gst_app_sink_try_pull_sample(app_sink, timeout);
  // sample will be NULL if timeout expires or on EOS/error
}

Napi::Promise::Deferred PullSampleWorker::GetPromise() { 
  return deferred; 
}

void PullSampleWorker::OnOK() {
  Napi::HandleScope scope(Env());

  if (sample) {
    Napi::Object result = TypeConversion::gst_sample_to_js(Env(), sample);
    deferred.Resolve(result);
    return;
  }

  // Timeout or no sample/error
  deferred.Resolve(Env().Null());
}

void PullSampleWorker::OnError(const Napi::Error &error) {
  Napi::HandleScope scope(Env());
  deferred.Reject(error.Value());
}

void PullSampleWorker::cleanup() {
  // Clean up resources - safe to call multiple times
  if (sample) {
    gst_sample_unref(sample);
    sample = nullptr;
  }
  if (app_sink) {
    gst_object_unref(app_sink);
    app_sink = nullptr;
  }
}

// StateChangeWorker implementation
StateChangeWorker::StateChangeWorker(Napi::Env env, GstPipeline *pipeline, GstState target_state, GstClockTime timeout) :
    Napi::AsyncWorker(env), pipeline(pipeline), target_state(target_state), timeout(timeout),
    state_change_result(GST_STATE_CHANGE_FAILURE), final_state(GST_STATE_VOID_PENDING), deferred(env) {
  // Increase reference count since we'll be using this in another thread
  gst_object_ref(pipeline);
}

StateChangeWorker::~StateChangeWorker() { 
  cleanup(); 
}

void StateChangeWorker::Execute() {
  // Set the state
  state_change_result = gst_element_set_state(GST_ELEMENT(pipeline), target_state);
  
  if (state_change_result == GST_STATE_CHANGE_FAILURE) {
    // State change failed immediately
    return;
  }
  
  // Wait for the state change to complete
  GstState pending;
  state_change_result = gst_element_get_state(GST_ELEMENT(pipeline), &final_state, &pending, timeout);
  
  // state_change_result will be:
  // - GST_STATE_CHANGE_SUCCESS: State change completed successfully
  // - GST_STATE_CHANGE_ASYNC: State change is in progress (we timed out)
  // - GST_STATE_CHANGE_FAILURE: State change failed
}

Napi::Promise::Deferred StateChangeWorker::GetPromise() { 
  return deferred; 
}

void StateChangeWorker::OnOK() {
  Napi::HandleScope scope(Env());
  
  Napi::Object result = Napi::Object::New(Env());
  
  // Include the state change result
  std::string result_str;
  switch (state_change_result) {
    case GST_STATE_CHANGE_SUCCESS:
      result_str = "success";
      break;
    case GST_STATE_CHANGE_ASYNC:
      result_str = "async";
      break;
    case GST_STATE_CHANGE_NO_PREROLL:
      result_str = "no-preroll";
      break;
    case GST_STATE_CHANGE_FAILURE:
      result_str = "failure";
      break;
    default:
      result_str = "unknown";
      break;
  }
  
  result.Set("result", Napi::String::New(Env(), result_str));
  result.Set("finalState", Napi::Number::New(Env(), final_state));
  result.Set("targetState", Napi::Number::New(Env(), target_state));
  
  deferred.Resolve(result);
}

void StateChangeWorker::OnError(const Napi::Error &error) {
  Napi::HandleScope scope(Env());
  deferred.Reject(error.Value());
}

void StateChangeWorker::cleanup() {
  if (pipeline) {
    gst_object_unref(pipeline);
    pipeline = nullptr;
  }
} 