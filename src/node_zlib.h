/**
 * ZLib binding for node.js
 */
#ifndef NODE_ZLIB_H
#define NODE_ZLIB_H

#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>

#include <zlib.h>

using namespace node;
using namespace v8;

class ZLibContext : public ObjectWrap {
  public:

    static void Initialize(Handle<Object> target);

    static Persistent<FunctionTemplate> constructor_template;

  protected:
    ZLibContext() : ObjectWrap(), deflate_s(), inflate_s() {
      deflate_s.zalloc = inflate_s.zalloc = Z_NULL;
      deflate_s.zfree  = inflate_s.zfree  = Z_NULL;
      deflate_s.opaque = inflate_s.opaque = Z_NULL;

      deflateInit(&deflate_s, Z_DEFAULT_COMPRESSION);
      inflateInit(&inflate_s);

      dictSet = hasDict = false;
      dictLen = 0;
    }

    ~ZLibContext() { }


    static Handle<Value> New(const Arguments &args);

    static Handle<Value> Deflate(const Arguments &args);
    static Handle<Value> Inflate(const Arguments &args);

    static Handle<Value> ResetDeflate(const Arguments &args);
    static Handle<Value> ResetInflate(const Arguments &args);

    z_stream deflate_s;
    z_stream inflate_s;

    bool hasDict;
    bool dictSet;
    int dictLen;
    Bytef* bdict;
    Persistent<Object> pdict;

};

#endif // NODE_ZLIB_H
