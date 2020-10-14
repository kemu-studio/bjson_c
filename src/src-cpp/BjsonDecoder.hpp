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

#ifndef _BjsonDecoder_Hpp_
#define _BjsonDecoder_Hpp_

#include <bjson/bjson-common.h>
#include <bjson/bjson-decode.h>

// -----------------------------------------------------------------------------
//       Helper macros to wrap pure C calbacks into C++ member calls
// -----------------------------------------------------------------------------

#define BJSON_CPP_DECODE0(NAME)                              \
  static bjson_decoderCallbackResult_t                       \
    _handlerForPureC_##NAME(void *thiz)                      \
  {                                                          \
    return (reinterpret_cast<BjsonDecoder*>(thiz))->NAME();  \
  }

#define BJSON_CPP_DECODE1(NAME, TYPE1)                           \
  static bjson_decoderCallbackResult_t                           \
    _handlerForPureC_##NAME(void *thiz, TYPE1 value)             \
  {                                                              \
    return (reinterpret_cast<BjsonDecoder*>(thiz))->NAME(value); \
  }

#define BJSON_CPP_DECODE2(NAME, TYPE1, TYPE2)                      \
  static bjson_decoderCallbackResult_t                             \
    _handlerForPureC_##NAME(void *thiz, TYPE1 x, TYPE2 y)          \
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

  virtual bjson_decoderCallbackResult_t onDefault() {return bjson_decoderCallbackResult_Continue;}
  virtual bjson_decoderCallbackResult_t onNull() {return onDefault();}
  virtual bjson_decoderCallbackResult_t onBoolean(int /*unused*/) {return onDefault();}
  virtual bjson_decoderCallbackResult_t onInteger(int64_t /*unused*/) {return onDefault();}
  virtual bjson_decoderCallbackResult_t onDouble(double /*unused*/) {return onDefault();}
  virtual bjson_decoderCallbackResult_t onNumber(const unsigned char * /*unused*/, size_t /*unused*/) {return onDefault();}
  virtual bjson_decoderCallbackResult_t onString(const unsigned char * /*unused*/, size_t /*unused*/) {return onDefault();}
  virtual bjson_decoderCallbackResult_t onMapKey(const unsigned char * /*unused*/, size_t /*unused*/) {return onDefault();}
  virtual bjson_decoderCallbackResult_t onStartMap() {return onDefault();}
  virtual bjson_decoderCallbackResult_t onEndMap() {return onDefault();}
  virtual bjson_decoderCallbackResult_t onStartArray() {return onDefault();}
  virtual bjson_decoderCallbackResult_t onEndArray() {return onDefault();}

  // ---------------------------------------------------------------------------
  //                        Internal wrappers (private)
  // ---------------------------------------------------------------------------

private:

  char *              _errorMsg = nullptr;
  bjson_decodeCtx_t * _ctx      = nullptr;

  BJSON_CPP_DECODE0(onNull)
  BJSON_CPP_DECODE0(onStartMap)
  BJSON_CPP_DECODE0(onEndMap)
  BJSON_CPP_DECODE0(onStartArray)
  BJSON_CPP_DECODE0(onEndArray)

  BJSON_CPP_DECODE1(onBoolean, int)
  BJSON_CPP_DECODE1(onInteger, int64_t)
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

  virtual void _init()
  {
    _errorMsg = nullptr;
    _ctx      = bjson_decoderCreate(&_callbacks, nullptr, this);
  }

  BjsonDecoder()
  {
    _init();
  }

  BjsonDecoder(BjsonDecoder const&)             = default;
  BjsonDecoder& operator =(BjsonDecoder const&) = default;
  BjsonDecoder(BjsonDecoder&&)                  = default;
  BjsonDecoder& operator=(BjsonDecoder&&)       = default;

  // ---------------------------------------------------------------------------
  //                      Wrap clean code into destructor
  // ---------------------------------------------------------------------------

  virtual void _clear()
  {
    if (_errorMsg != nullptr)
    {
      bjson_decoderFreeErrorMessage(_ctx, _errorMsg);
    }

    if (_ctx != nullptr)
    {
      bjson_decoderDestroy(_ctx);

      _ctx = nullptr;
    }
  }

  virtual ~BjsonDecoder()
  {
    _clear();
  }

  // ---------------------------------------------------------------------------
  //                               Public API
  // ---------------------------------------------------------------------------

  bjson_status_t parse(const void *buf, size_t bufSize)
  {
    return bjson_decoderParse(_ctx, buf, bufSize);
  }

  bjson_status_t complete()
  {
    return bjson_decoderComplete(_ctx);
  }

  void reset()
  {
    // Possible improvement: Optimize it.
    _clear();
    _init();
  }

  // ---------------------------------------------------------------------------
  //                Wrappers for status management functions
  // ---------------------------------------------------------------------------

  inline const char *formatErrorMessage(int verbose)
  {
    if (_errorMsg != nullptr)
    {
      bjson_decoderFreeErrorMessage(_ctx, _errorMsg);
    }
    _errorMsg = bjson_decoderFormatErrorMessage(_ctx, verbose);

    return _errorMsg;
  }

  // ---------------------------------------------------------------------------
  //               Wrappers for version related functions
  // ---------------------------------------------------------------------------

  static inline const char * getVersionAsText() { return bjson_getVersionAsText(); }
  static inline unsigned int getVersion()       { return bjson_getVersion(); }
};

#endif /* _BjsonDecoder_Hpp_ */
