#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <node_version.h>

#include <zlib.h>
#include <cstring>


using namespace v8;
using namespace node;


// node v0.2 compatibility
#if NODE_VERSION_AT_LEAST(0,3,0)
    #define Buffer_Data Buffer::Data
    #define Buffer_Length Buffer::Length
    #define Buffer_New Buffer::New
#else
    inline char* Buffer_Data(Handle<Object> obj) {
        return (ObjectWrap::Unwrap<Buffer>(obj))->data();
    }
    inline size_t Buffer_Length(Handle<Object> obj) {
        return (ObjectWrap::Unwrap<Buffer>(obj))->length();
    }
    inline Buffer* Buffer_New(char* data, size_t length) {
        Buffer* buffer = Buffer::New(length);
        memcpy(buffer->data(), data, length);
        return buffer;
    }
#endif


z_stream deflate_s;

class ZLib {
public:
    static inline Handle<Value> Error(const char* msg = NULL) {
        return ThrowException(Exception::Error(
            String::New(msg ? msg : "Unknown Error")));
    }

    static Handle<Value> Deflate(const Arguments& args) {
        HandleScope scope;

        if (args.Length() < 1 || !Buffer::HasInstance(args[0])) {
            return Error("Expected Buffer as first argument");
        }

        Local<Object> input = args[0]->ToObject();
        deflate_s.next_in = (Bytef*)Buffer_Data(input);
        int length = deflate_s.avail_in = Buffer_Length(input);

        int status;
        char* result = NULL;

        int compressed = 0;
        do {
            result = (char*)realloc(result, compressed + length);
            if (!result) return Error("Could not allocate memory");

            deflate_s.avail_out = length;
            deflate_s.next_out = (Bytef*)result + compressed;

            status = deflate(&deflate_s, Z_FINISH);
            if (status != Z_STREAM_END && status != Z_OK) {
                free(result);
                return Error(deflate_s.msg);
            }

            compressed += (length - deflate_s.avail_out);
        } while (deflate_s.avail_out == 0);

        status = deflateReset(&deflate_s);
        if (status != Z_OK) {
            free(result);
            return Error(deflate_s.msg);
        }

        Buffer* output = Buffer_New(result, compressed);
        free(result);
        return scope.Close(Local<Value>::New(output->handle_));
    }
};


extern "C" void init (Handle<Object> target) {
    deflate_s.zalloc = Z_NULL;
    deflate_s.zfree = Z_NULL;
    deflate_s.opaque = Z_NULL;
    deflateInit(&deflate_s, Z_DEFAULT_COMPRESSION);

    NODE_SET_METHOD(target, "deflate", ZLib::Deflate);
}
