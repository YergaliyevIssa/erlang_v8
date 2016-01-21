#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

#include "include/libplatform/libplatform.h"
#include "include/v8.h"

#include "debug.h"
#include "packet.h"
#include "report.h"

using namespace v8;
using namespace std;

const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}

void Report(Isolate* isolate, Handle<Value> response, uint8_t op) {
    uint32_t ref = 0;

    Handle<Value> input;

    TRACE("REPORTING STUFF.");

    if (response->IsUndefined()) {
        TRACE("Undefined");
        input = String::NewFromUtf8(isolate, "undefined");
    } else {
        input = JSONStringify(isolate, response);
    }

    String::Utf8Value utf8 (input);
    uint32_t len = utf8.length() + 1 + 4;

    cout << (uint8_t)((len >> 24) & 0xff);
    cout << (uint8_t)((len >> 16) & 0xff);
    cout << (uint8_t)((len >> 8) & 0xff);
    cout << (uint8_t)(len & 0xff);

    cout << op;

    cout << (uint8_t)((ref >> 24) & 0xff);
    cout << (uint8_t)((ref >> 16) & 0xff);
    cout << (uint8_t)((ref >> 8) & 0xff);
    cout << (uint8_t)(ref & 0xff);

    cout << *utf8;
    cout.flush();
}

void ReportOK(Isolate* isolate, Handle<Value> response) {
    Report(isolate, response, OP_OK);
}

void ReportError(Isolate* isolate, Handle<Value> response) {
    Report(isolate, WrapError(isolate, response), OP_ERROR);
}

void ReportException(Isolate* isolate, TryCatch* try_catch) {
    HandleScope handle_scope(isolate);
    Handle<Value> stack_trace = try_catch->StackTrace();

    if (stack_trace.IsEmpty()) {
        ReportError(isolate, try_catch->Exception());
    } else {
        const char* st = ToCString(String::Utf8Value(try_catch->StackTrace()));
        FTRACE("Stack: %s\n", st);
        ReportError(isolate, try_catch->StackTrace());
    }
}

Handle<Value> WrapError(Isolate* isolate, Handle<Value> exception) {
    EscapableHandleScope handle_scope(isolate);

    Local<Object> obj = Object::New(isolate);

    String::Utf8Value exception_string(exception);
    std::string from = std::string(*exception_string);

    obj->Set(String::NewFromUtf8(isolate, "error"),
             exception->ToString());

    // add line number and other fancy details ...

    return handle_scope.Escape(obj);
}

Handle<Value> JSONStringify(Isolate* isolate, Handle<Value> obj) {
    Handle<Context> context = isolate->GetCurrentContext();
    Handle<Object> global = context->Global();
    EscapableHandleScope handle_scope(isolate);

    Handle<Value> JSONValue = global->Get(String::NewFromUtf8(isolate, "JSON"));
    Handle<Object> JSON = JSONValue->ToObject();
    Handle<Function> stringify = Handle<Function>::Cast(
            JSON->Get(String::NewFromUtf8(isolate, "stringify")));

    Handle<Value> args[] = { obj };
    Local<Value> result = stringify->Call(JSON, 1, args);

    return handle_scope.Escape(result);
}
