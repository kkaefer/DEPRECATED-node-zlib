#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <node_version.h>

#include <zlib.h>

using namespace v8;
using namespace node;

z_stream stream;

class ZLib {
public:

    static Handle<Value> Error() {
        if (stream.msg) {
            return ThrowException(Exception::Error(String::New(stream.msg)));
        }
        else {
            return ThrowException(Exception::Error(String::New("Unknown error")));
        }
    }

    static Handle<Value> Error(const char* msg) {
        return ThrowException(Exception::Error(String::New(msg)));
    }

    static Handle<Value> Deflate(const Arguments& args) {
        HandleScope scope;

        if (args.Length() < 1 || !Buffer::HasInstance(args[0])) {
            return Error("Expected Buffer as first argument");
        }

#if NODE_VERSION_AT_LEAST(0,3,0)
        Local<Object> buffer = args[0]->ToObject();
        stream.next_in = (Bytef*)Buffer::Data(buffer);
        int length = stream.avail_in = Buffer::Length(buffer);
#else
        Buffer* buffer = ObjectWrap::Unwrap<Buffer>(args[0]->ToObject());
        stream.next_in = (Bytef*)buffer->data();
        int length = stream.avail_in = buffer->length();
#endif

        int status;
        char* result = NULL;

        int compressed = 0;
        do {
            result = (char*)realloc(result, compressed + length);
            if (!result) return Error("Failed to allocate memory");

            stream.avail_out = length;
            stream.next_out = (Bytef*)result + compressed;

            status = deflate(&stream, Z_FINISH);
            if (status != Z_STREAM_END && status != Z_OK) return Error();

            compressed += (length - stream.avail_out);
        } while (stream.avail_out == 0);

        status = deflateReset(&stream);
        if (status != Z_OK) return Error();

#if NODE_VERSION_AT_LEAST(0,3,0)
        Buffer* resultBuffer = Buffer::New(result, compressed);
#else
        Buffer *resultBuffer = Buffer::New(compressed);
        memcpy(buffer->data(), result, compressed);
#endif
        free(result);

        return scope.Close(Local<Value>::New(resultBuffer->handle_));
    }
};


extern "C" void init (v8::Handle<Object> target) {
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    deflateInit(&stream, Z_DEFAULT_COMPRESSION);

    NODE_SET_METHOD(target, "deflate", ZLib::Deflate);
}
