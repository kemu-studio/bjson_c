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

#include "BjsonCommon.hpp"
#include "../bjson-decode.h"

class BjsonDecoder
{
  private:

  bjson_encodeCtx_t *_bjsonEncodeCtx;

  public:

  BjsonEncoder()
  {
    _bjsonEncodeCtx = bjson_encoderCreate(NULL, NULL);
  }

  ~BjsonEncoder()
  {
    if (_bjsonEncodeCtx)
    {
      bjson_encoderDestroy(_bjsonEncodeCtx);

      _bjsonEncodeCtx = NULL;
    }
  }

// TODO: Special cases:
//  CREATE_BJSON_CPP_WRAPPER2(getResult , bjson_encoderGetResult , void **, buf, size_t *, bufSize)
//  CREATE_BJSON_CPP_WRAPPER0(clear     , bjson_encoderClear)
//  CREATE_BJSON_CPP_WRAPPER1(reset     , bjson_encoderReset     , const char *, sepText)

  CREATE_BJSON_CPP_WRAPPER1(encodeInteger, long long int, value)
  CREATE_BJSON_CPP_WRAPPER1(encodeDouble, double, value)
  CREATE_BJSON_CPP_WRAPPER0(encodeNull)
  CREATE_BJSON_CPP_WRAPPER1(encodeBool, int, value)

  CREATE_BJSON_CPP_WRAPPER0(encodeMapOpen)
  CREATE_BJSON_CPP_WRAPPER0(encodeMapClose)
  CREATE_BJSON_CPP_WRAPPER0(encodeArrayOpen)
  CREATE_BJSON_CPP_WRAPPER0(encodeArrayClose)

  CREATE_BJSON_CPP_WRAPPER2(encodeNumberFromText, const char *, text, size_t, textLen)
  CREATE_BJSON_CPP_WRAPPER2(encodeString        , const char *, text, size_t, textLen)
  CREATE_BJSON_CPP_WRAPPER1(encodeCString       , const char *, text)
  CREATE_BJSON_CPP_WRAPPER2(encodeBinary        , void *, blob, size_t, blobSize)

  CREATE_BJSON_CPP_WRAPPER0(encoderGetStatus)

  // TODO
  //BJSON_API char *
  //  bjson_encoderFormatErrorMessage(bjson_encodeCtx_t *ctx, int verbose);

  //BJSON_API void
  //  bjson_encoderFreeErrorMessage(bjson_encodeCtx_t *ctx, char *errorMsg);
};
