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

#ifndef _BjsonEncoder_Hpp_
#define _BjsonEncoder_Hpp_

#include <bjson/bjson-common.h>
#include <bjson/bjson-encode.h>

// -----------------------------------------------------------------------------
//        Helper macros to wrap pure C calbacks into C++ member calls
// -----------------------------------------------------------------------------

// Wrap C call bjson_encodeXxx(thiz) into:
// - C++ method: thiz->encodeXxx(),
// - C++ method: this->encodeKeyAndValueXxx()

#define BJSON_CPP_ENCODE0(NAME)                   \
  bjson_status_t encode##NAME()                   \
  {                                               \
    return bjson_encode##NAME(_ctx);              \
  }                                               \
                                                  \
  inline bjson_status_t encodeKeyAndValue##NAME(  \
                                 const char *key) \
  {                                               \
    encodeCString(key);                           \
    return encode##NAME();                        \
  }

// Wrap C call bjson_encodeXxx(thiz, x) into C++ methods:
// - thiz->encodeXxx(x),
// - this->encodeKeyAndValueXxx(key, x).

#define BJSON_CPP_ENCODE1(NAME, TYPE1)            \
  bjson_status_t encode##NAME(TYPE1 x)            \
  {                                               \
    return bjson_encode##NAME(_ctx, x);           \
  }                                               \
                                                  \
  inline bjson_status_t encodeKeyAndValue##NAME(  \
                                const char *key,  \
                                        TYPE1 x)  \
  {                                               \
    encodeCString(key);                           \
    return encode##NAME(x);                       \
  }

// Wrap C call bjson_encodeXxx(thiz, x, y) into:
// - C++ method: thiz->encodeXxx(x, y),
// - C++ method: this->encodeKeyAndValueXxx(key, x, y).

#define BJSON_CPP_ENCODE2(NAME, TYPE1, TYPE2)     \
  bjson_status_t encode##NAME(TYPE1 x, TYPE2 y)   \
  {                                               \
    return bjson_encode##NAME(_ctx, x, y);        \
  }                                               \
                                                  \
  inline bjson_status_t encodeKeyAndValue##NAME(  \
                               const char *key,   \
                                       TYPE1 x,   \
                                       TYPE2 y)   \
  {                                               \
    encodeCString(key);                           \
    return encode##NAME(x, y);                    \
  }

class BjsonEncoder
{

private:

  char *             _errorMsg = nullptr;
  bjson_encodeCtx_t *_ctx      = nullptr;

public:

  // ---------------------------------------------------------------------------
  //                      Wrap init code into constructor
  // ---------------------------------------------------------------------------

  BjsonEncoder() {
    _errorMsg = nullptr;
    _ctx      = bjson_encoderCreate(nullptr, nullptr);
  }

  BjsonEncoder(BjsonEncoder const&)             = default;
  BjsonEncoder& operator =(BjsonEncoder const&) = default;
  BjsonEncoder(BjsonEncoder&&)                  = default;
  BjsonEncoder& operator=(BjsonEncoder&&)       = default;

  // ---------------------------------------------------------------------------
  //                     Wrap clean code into destructor
  // ---------------------------------------------------------------------------

  virtual ~BjsonEncoder()
  {
    if (_errorMsg != nullptr)
    {
      bjson_encoderFreeErrorMessage(_ctx, _errorMsg);
    }

    if (_ctx != nullptr)
    {
      bjson_encoderDestroy(_ctx);
      _ctx = nullptr;
    }
  }

  // ---------------------------------------------------------------------------
  //             Wrappers for state/result management functions
  // ---------------------------------------------------------------------------

  bjson_status_t getResult(void **buf, size_t *bufSize) { return bjson_encoderGetResult(_ctx, buf, bufSize); }
  bjson_status_t clear()                                { return bjson_encoderClear(_ctx);                   }
  bjson_status_t reset(const char *sepText)             { return bjson_encoderReset(_ctx, sepText);          }

  // ---------------------------------------------------------------------------
  //               Wrappers for zero-args encode functions
  // ---------------------------------------------------------------------------

  BJSON_CPP_ENCODE0(Null)
  BJSON_CPP_ENCODE0(MapOpen)
  BJSON_CPP_ENCODE0(MapClose)
  BJSON_CPP_ENCODE0(ArrayOpen)
  BJSON_CPP_ENCODE0(ArrayClose)

  // ---------------------------------------------------------------------------
  //                Wrappers for one-arg encode functions
  // ---------------------------------------------------------------------------

  BJSON_CPP_ENCODE1(Integer, int64_t)
  BJSON_CPP_ENCODE1(Double, double)
  BJSON_CPP_ENCODE1(Bool, int)
  BJSON_CPP_ENCODE1(CString, const char *)

  // ---------------------------------------------------------------------------
  //                Wrappers for two-args encode functions
  // ---------------------------------------------------------------------------

  BJSON_CPP_ENCODE2(NumberFromText, const char *, size_t)
  BJSON_CPP_ENCODE2(String, const char *, size_t)
  BJSON_CPP_ENCODE2(Binary, void *, size_t)

  // ---------------------------------------------------------------------------
  //              Wrappers for status management functions
  // ---------------------------------------------------------------------------

  bjson_status_t getStatus()
  {
    return bjson_encoderGetStatus(_ctx);
  }

  inline const char *formatErrorMessage(int verbose)
  {
    if (_errorMsg != nullptr)
    {
      bjson_encoderFreeErrorMessage(_ctx, _errorMsg);
    }
    _errorMsg = bjson_encoderFormatErrorMessage(_ctx, verbose);

    return _errorMsg;
  }

  // ---------------------------------------------------------------------------
  //               Wrappers for version related functions
  // ---------------------------------------------------------------------------

  static inline const char * getVersionAsText() { return bjson_getVersionAsText(); }
  static inline unsigned int getVersion()       { return bjson_getVersion(); }
};

#endif /* _BjsonEncoder_Hpp_ */
