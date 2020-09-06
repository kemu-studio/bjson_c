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
#include <bjson/bjson-encode.h>

// -----------------------------------------------------------------------------
//        Helper macros to wrap pure C calbacks into C++ member calls
// -----------------------------------------------------------------------------

#define BJSON_CPP_ENCODE0(NAME) \
  bjson_status_t NAME()         \
  {                             \
    return bjson_##NAME(_ctx);  \
  }

#define BJSON_CPP_ENCODE1(NAME, TYPE1) \
  bjson_status_t NAME(TYPE1 x)         \
  {                                    \
    return bjson_##NAME(_ctx, x);      \
  }

#define BJSON_CPP_ENCODE2(NAME, TYPE1, TYPE2) \
  bjson_status_t NAME(TYPE1 x, TYPE2 y)       \
  {                                           \
    return bjson_##NAME(_ctx, x, y);          \
  }

class BjsonEncoder
{

private:
  char *_errorMsg{} {};

  bjson_encodeCtx_t *_ctx{} {};

public:

  // ---------------------------------------------------------------------------
  //                      Wrap init code into constructor
  // ---------------------------------------------------------------------------

  BjsonEncoder() {
    _errorMsg = NULL;
    _ctx      = bjson_encoderCreate(NULL, NULL);
  }

  // ---------------------------------------------------------------------------
  //                     Wrap clean code into destructor
  // ---------------------------------------------------------------------------

  virtual ~BjsonEncoder()
  {
    if (_errorMsg != nullptr)
    {
      bjson_encoderFreeErrorMessage(_ctx, _errorMsg);
    }

    if (_ctx)
    {
      bjson_encoderDestroy(_ctx);
      _ctx = NULL;
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

  BJSON_CPP_ENCODE0(encodeNull)
  BJSON_CPP_ENCODE0(encodeMapOpen)
  BJSON_CPP_ENCODE0(encodeMapClose)
  BJSON_CPP_ENCODE0(encodeArrayOpen)
  BJSON_CPP_ENCODE0(encodeArrayClose)

  // ---------------------------------------------------------------------------
  //                Wrappers for one-arg encode functions
  // ---------------------------------------------------------------------------

  BJSON_CPP_ENCODE1(encodeInteger, long long int)
  BJSON_CPP_ENCODE1(encodeDouble, double)
  BJSON_CPP_ENCODE1(encodeBool, int)
  BJSON_CPP_ENCODE1(encodeCString, const char *)

  // ---------------------------------------------------------------------------
  //                Wrappers for two-args encode functions
  // ---------------------------------------------------------------------------

  BJSON_CPP_ENCODE2(encodeNumberFromText, const char *, size_t)
  BJSON_CPP_ENCODE2(encodeString, const char *, size_t)
  BJSON_CPP_ENCODE2(encodeBinary, void *, size_t)

  // ---------------------------------------------------------------------------
  //              Wrappers for status management functions
  // ---------------------------------------------------------------------------

  bjson_status_t getStatus(const char *sepText)
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

  inline const char * getVersionAsText() {return bjson_getVersionAsText();}
  inline unsigned int getVersion()       {return bjson_getVersion();}
};
