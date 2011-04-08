#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include <zlib.h>
#include <cstring>
#include <cstdlib>

#include "node_zlib.h"

using namespace v8;
using namespace node;

Persistent<FunctionTemplate> ZLibContext::constructor_template;

void ZLibContext::Initialize(Handle<Object> target) {
  Local<FunctionTemplate> t = FunctionTemplate::New(ZLibContext::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("ZLibContext"));

  // Instance methods
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "deflate",
                            ZLibContext::Deflate);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "inflate",
                            ZLibContext::Inflate);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "resetDeflate",
                            ZLibContext::ResetDeflate);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "resetInflate",
                            ZLibContext::ResetInflate);

  target->Set(String::NewSymbol("ZLibContext"), constructor_template->GetFunction());
};

Handle<Value> ZLibContext::New(const Arguments &args) {
  if (!args.IsConstructCall()) {
    return FromConstructorTemplate(constructor_template, args);
  }

  HandleScope scope;

  ZLibContext *z = new ZLibContext();
  z->Wrap(args.This());

  // First argument can be initial dictionary
  if (args.Length() >= 1 && Buffer::HasInstance(args[0])) {
    Local<Object> dict = args[0]->ToObject();
    z->pdict = Persistent<Object>::New(dict);
    z->bdict = (Bytef*)Buffer::Data(dict);
    z->dictLen = Buffer::Length(dict);
    z->hasDict = true;
  }

  return args.This();
};

inline Handle<Value> ZLib_error(const char* msg = NULL) {
    return ThrowException(Exception::Error(
        String::New(msg ? msg : "Unknown Error")));
};

#define ZLib_Xreset(Method, method)                                            \
Handle<Value> ZLibContext::Reset##Method(const Arguments& args) {              \
  HandleScope scope;                                                           \
  ZLibContext *z = ObjectWrap::Unwrap<ZLibContext>(args.This());               \
                                                                               \
  if (method##Reset(&(z->method##_s)) != Z_OK) {                               \
    return ZLib_error("ZLib stream is beyond repair");                         \
  }                                                                            \
                                                                               \
  z->dictSet = false;                                                          \
  return scope.Close(Boolean::New(true));                                      \
}

#define ZLib_Xflate(Method, method, factor)                                    \
Handle<Value> ZLibContext::Method(const Arguments& args) {                     \
  HandleScope scope;                                                           \
                                                                               \
  if (args.Length() < 1 || !Buffer::HasInstance(args[0])) {                    \
    return ZLib_error("Expected Buffer as first argument");                    \
  }                                                                            \
                                                                               \
  Local<Object> input = args[0]->ToObject();                                   \
                                                                               \
  ZLibContext *z = ObjectWrap::Unwrap<ZLibContext>(args.This());               \
                                                                               \
  int ret;                                                                     \
  if (factor == 1 && z->hasDict && !z->dictSet) {                              \
    ret = method##SetDictionary(&(z->method##_s), z->bdict, z->dictLen);       \
                                                                               \
    if (ret != Z_OK) {                                                         \
      return ZLib_error("Failed to set dictionary!");                          \
    }                                                                          \
                                                                               \
    z->dictSet = true;                                                         \
  }                                                                            \
                                                                               \
  z->method##_s.next_in = (Bytef*)Buffer::Data(input);                         \
  int length = z->method##_s.avail_in = Buffer::Length(input);                 \
                                                                               \
  char* result = NULL;                                                         \
                                                                               \
  int compressed = 0;                                                          \
  do {                                                                         \
    result = (char*)realloc(result, compressed + factor * length);             \
    if (!result) return ZLib_error("Could not allocate memory");               \
                                                                               \
    z->method##_s.avail_out = factor * length;                                 \
    z->method##_s.next_out = (Bytef*)result + compressed;                      \
                                                                               \
    ret = method(&(z->method##_s), Z_SYNC_FLUSH);                              \
                                                                               \
    if (ret == Z_NEED_DICT) {                                                  \
      if (!z->hasDict) {                                                       \
        free(result);                                                          \
        return ZLib_error("Dictionary is required");                           \
      }                                                                        \
      ret = method##SetDictionary(&(z->method##_s), z->bdict,                  \
                                  z->dictLen);                                 \
                                                                               \
      if (ret != Z_OK) {                                                       \
        return ZLib_error("Failed to set dictionary");                         \
      }                                                                        \
                                                                               \
      ret = method(&(z->method##_s), Z_SYNC_FLUSH);                            \
    }                                                                          \
                                                                               \
    if (ret != Z_STREAM_END && ret != Z_OK && ret != Z_BUF_ERROR) {            \
      free(result);                                                            \
      return ZLib_error(z->method##_s.msg);                                    \
    }                                                                          \
                                                                               \
    compressed += (factor * length - z->method##_s.avail_out);                 \
  } while (z->method##_s.avail_out == 0);                                      \
                                                                               \
  Buffer* output = Buffer::New(result, compressed);                            \
  free(result);                                                                \
  return scope.Close(Local<Value>::New(output->handle_));                      \
}

ZLib_Xflate(Deflate, deflate, 1);
ZLib_Xflate(Inflate, inflate, 2);
ZLib_Xreset(Deflate, deflate);
ZLib_Xreset(Inflate, inflate);

NODE_MODULE(zlib_bindings, ZLibContext::Initialize);
