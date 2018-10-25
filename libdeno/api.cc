// Copyright 2018 the Deno authors. All rights reserved. MIT license.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "third_party/v8/include/libplatform/libplatform.h"
#include "third_party/v8/include/v8.h"
#include "third_party/v8/src/base/logging.h"

#include "deno.h"
#include "internal.h"

extern "C" {

Deno* deno_new(deno_buf snapshot, deno_buf shared, deno_recv_cb cb) {
  deno::DenoIsolate* d = new deno::DenoIsolate(snapshot, cb, shared);
  v8::Isolate::CreateParams params;
  params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  params.external_references = deno::external_references;

  if (snapshot.data_ptr) {
    params.snapshot_blob = &d->snapshot_;
  }

  v8::Isolate* isolate = v8::Isolate::New(params);
  d->AddIsolate(isolate);

  v8::Locker locker(isolate);
  v8::Isolate::Scope isolate_scope(isolate);
  {
    v8::HandleScope handle_scope(isolate);
    auto context =
        v8::Context::New(isolate, nullptr, v8::MaybeLocal<v8::ObjectTemplate>(),
                         v8::MaybeLocal<v8::Value>(),
                         v8::DeserializeInternalFieldsCallback(
                             deno::DeserializeInternalFields, nullptr));
    d->context_.Reset(isolate, context);
  }

  return reinterpret_cast<Deno*>(d);
}

Deno* deno_new_snapshotter(deno_buf shared, deno_recv_cb cb,
                           const char* js_filename, const char* js_source,
                           const char* source_map) {
  auto* creator = new v8::SnapshotCreator(deno::external_references);
  auto* isolate = creator->GetIsolate();
  auto* d = new deno::DenoIsolate(deno::empty_buf, cb, shared);
  d->snapshot_creator_ = creator;
  d->AddIsolate(isolate);
  v8::Isolate::Scope isolate_scope(isolate);
  {
    v8::HandleScope handle_scope(isolate);
    auto context = v8::Context::New(isolate);
    creator->SetDefaultContext(context,
                               v8::SerializeInternalFieldsCallback(
                                   deno::SerializeInternalFields, nullptr));
    deno::InitializeContext(isolate, context, js_filename, js_source,
                            source_map);
  }
  return reinterpret_cast<Deno*>(d);
}

deno::DenoIsolate* unwrap(Deno* d_) {
  return reinterpret_cast<deno::DenoIsolate*>(d_);
}

deno_buf deno_get_snapshot(Deno* d_) {
  auto* d = unwrap(d_);
  CHECK_NE(d->snapshot_creator_, nullptr);
  auto blob = d->snapshot_creator_->CreateBlob(
      v8::SnapshotCreator::FunctionCodeHandling::kClear);
  return {nullptr, 0, reinterpret_cast<uint8_t*>(const_cast<char*>(blob.data)),
          blob.raw_size};
}

void deno_init() {
  // v8::V8::InitializeICUDefaultLocation(argv[0]);
  // v8::V8::InitializeExternalStartupData(argv[0]);
  auto* p = v8::platform::CreateDefaultPlatform();
  v8::V8::InitializePlatform(p);
  v8::V8::Initialize();
}

const char* deno_v8_version() { return v8::V8::GetVersion(); }

void deno_set_v8_flags(int* argc, char** argv) {
  v8::V8::SetFlagsFromCommandLine(argc, argv, true);
}

const char* deno_last_exception(Deno* d_) {
  auto* d = unwrap(d_);
  return d->last_exception_.c_str();
}

int deno_execute(Deno* d_, void* user_data_, const char* js_filename,
                 const char* js_source) {
  auto* d = unwrap(d_);
  deno::UserDataScope user_data_scope(d, user_data_);
  auto* isolate = d->isolate_;
  v8::Locker locker(isolate);
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  auto context = d->context_.Get(d->isolate_);
  return deno::Execute(context, js_filename, js_source) ? 1 : 0;
}

int deno_respond(Deno* d_, void* user_data_, int32_t req_id, deno_buf buf) {
  auto* d = unwrap(d_);
  if (d->current_args_ != nullptr) {
    // Synchronous response.
    auto ab = deno::ImportBuf(d, buf);
    d->current_args_->GetReturnValue().Set(ab);
    d->current_args_ = nullptr;
    return 0;
  }

  // Asynchronous response.
  deno::UserDataScope user_data_scope(d, user_data_);
  v8::Locker locker(d->isolate_);
  v8::Isolate::Scope isolate_scope(d->isolate_);
  v8::HandleScope handle_scope(d->isolate_);

  auto context = d->context_.Get(d->isolate_);
  v8::Context::Scope context_scope(context);

  v8::TryCatch try_catch(d->isolate_);

  deno::DeleteDataRef(d, req_id);

  auto recv_ = d->recv_.Get(d->isolate_);
  if (recv_.IsEmpty()) {
    d->last_exception_ = "libdeno.recv_ has not been called.";
    return 1;
  }

  v8::Local<v8::Value> args[1];
  args[0] = deno::ImportBuf(d, buf);
  recv_->Call(context->Global(), 1, args);

  if (try_catch.HasCaught()) {
    deno::HandleException(context, try_catch.Exception());
    return 1;
  }

  return 0;
}

void deno_check_promise_errors(Deno* d_) {
  auto* d = unwrap(d_);
  if (d->pending_promise_events_ > 0) {
    auto* isolate = d->isolate_;
    v8::Locker locker(isolate);
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    auto context = d->context_.Get(d->isolate_);
    v8::Context::Scope context_scope(context);

    v8::TryCatch try_catch(d->isolate_);
    auto promise_error_examiner_ = d->promise_error_examiner_.Get(d->isolate_);
    if (promise_error_examiner_.IsEmpty()) {
      d->last_exception_ =
          "libdeno.setPromiseErrorExaminer has not been called.";
      return;
    }
    v8::Local<v8::Value> args[0];
    auto result = promise_error_examiner_->Call(context->Global(), 0, args);
    if (try_catch.HasCaught()) {
      deno::HandleException(context, try_catch.Exception());
    }
    d->pending_promise_events_ = 0;  // reset
    if (!result->BooleanValue(context).FromJust()) {
      // Has uncaught promise reject error, exiting...
      exit(1);
    }
  }
}

void deno_delete(Deno* d_) {
  deno::DenoIsolate* d = reinterpret_cast<deno::DenoIsolate*>(d_);
  d->isolate_->Dispose();
  delete d;
}

void deno_terminate_execution(Deno* d_) {
  deno::DenoIsolate* d = reinterpret_cast<deno::DenoIsolate*>(d_);
  d->isolate_->TerminateExecution();
}
}
