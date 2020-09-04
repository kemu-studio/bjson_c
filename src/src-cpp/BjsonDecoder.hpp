/*
 * Copyright (c) 2017,2020 by Kemu Studio (visit ke.mu)
 *
 * Author(s): Sylwester Wysocki <sw@ke.mu>,
 *            Roman Pietrzak <rp@ke.mu>
 *
 * This file is a part of the KEMU Binary JSON library.
 * See http://bjson.org for more.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <bjson/bjson-common.h>
#include <bjson/bjson-decode.h>

// -----------------------------------------------------------------------------
//       Helper macros to wrap pure C calbacks into C++ member calls
// -----------------------------------------------------------------------------

#define BJSON_CPP_DECODE0(NAME)                              \
  static int _handlerForPureC_##NAME(void *thiz)             \
  {                                                          \
    return (reinterpret_cast<BjsonDecoder*>(thiz))->NAME();  \
  }

#define BJSON_CPP_DECODE1(NAME, TYPE1)                           \
  static int _handlerForPureC_##NAME(void *thiz, TYPE1 value)    \
  {                                                              \
    return (reinterpret_cast<BjsonDecoder*>(thiz))->NAME(value); \
  }

#define BJSON_CPP_DECODE2(NAME, TYPE1, TYPE2)                      \
  static int _handlerForPureC_##NAME(void *thiz, TYPE1 x, TYPE2 y) \
  {                                                                \
    return (reinterpret_cast<BjsonDecoder*>(thiz))->NAME(x, y);    \
  }


class BjsonDecoder
{
protected:

  // ---------------------------------------------------------------------------
  // Callback methods called when next token was successfuly decoded.
  // Overload one or more of below methods to catch decoded values
  // in stream-like fasion.
  // ---------------------------------------------------------------------------

  virtual int onNull() {}
  virtual int onBoolean(int) {}
  virtual int onInteger(long long) {}
  virtual int onDouble(double) {}
  virtual int onNumber(const unsigned char *, size_t) {}
  virtual int onString(const unsigned char *, size_t) {}
  virtual int onMapKey(const unsigned char *, size_t) {}
  virtual int onStartMap() {}
  virtual int onEndMap() {}
  virtual int onStartArray() {}
  virtual int onEndArray() {}

  // ---------------------------------------------------------------------------
  //                        Internal wrappers (private)
  // ---------------------------------------------------------------------------

private:

  char *_errorMsg;
  bjson_decodeCtx_t *_ctx;

  BJSON_CPP_DECODE0(onNull)
  BJSON_CPP_DECODE0(onStartMap)
  BJSON_CPP_DECODE0(onEndMap)
  BJSON_CPP_DECODE0(onStartArray)
  BJSON_CPP_DECODE0(onEndArray)

  BJSON_CPP_DECODE1(onBoolean, int)
  BJSON_CPP_DECODE1(onInteger, long long)
  BJSON_CPP_DECODE1(onDouble, double)

  BJSON_CPP_DECODE2(onNumber, const unsigned char *, size_t)
  BJSON_CPP_DECODE2(onString, const unsigned char *, size_t)
  BJSON_CPP_DECODE2(onMapKey, const unsigned char *, size_t)

  bjson_decoderCallbacks_t _callbacks
  {
    _handlerForPureC_onNull,
    _handlerForPureC_onBoolean,
    _handlerForPureC_onInteger,
    _handlerForPureC_onDouble,

    _handlerForPureC_onNumber,
    _handlerForPureC_onString,

    _handlerForPureC_onStartMap,
    _handlerForPureC_onMapKey,
    _handlerForPureC_onEndMap,

    _handlerForPureC_onStartArray,
    _handlerForPureC_onEndArray
  };

  public:

  // ---------------------------------------------------------------------------
  //                      Wrap init code into constructor
  // ---------------------------------------------------------------------------

  BjsonDecoder()
  {
    _errorMsg = NULL;
    _ctx      = bjson_decoderCreate(&_callbacks, NULL, this);
  }

  // ---------------------------------------------------------------------------
  //                      Wrap clean code into destructor
  // ---------------------------------------------------------------------------

  virtual ~BjsonDecoder()
  {
    if (_errorMsg)
    {
      bjson_decoderFreeErrorMessage(_ctx, _errorMsg);
    }

    if (_ctx)
    {
      bjson_decoderDestroy(_ctx);

      _ctx = NULL;
    }
  }

  // ---------------------------------------------------------------------------
  //                               Public API
  // ---------------------------------------------------------------------------

  bjson_status_t parse(void *buf, size_t bufSize)
  {
    return bjson_decoderParse(_ctx, buf, bufSize);
  }

  bjson_status_t complete()
  {
    return bjson_decoderComplete(_ctx);
  }

  // ---------------------------------------------------------------------------
  //                Wrappers for status management functions
  // ---------------------------------------------------------------------------

  inline const char *formatErrorMessage(int verbose)
  {
    if (_errorMsg)
    {
      bjson_decoderFreeErrorMessage(_ctx, _errorMsg);
    }
    _errorMsg = bjson_decoderFormatErrorMessage(_ctx, verbose);

    return _errorMsg;
  }

  // ---------------------------------------------------------------------------
  //               Wrappers for version related functions
  // ---------------------------------------------------------------------------

  inline const char * getVersionAsText() {return bjson_getVersionAsText();}
  inline unsigned int getVersion()       {return bjson_getVersion();}
};
