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

//
// This is a basic example showing how to decode input BJSON stream into
// list of tokens.
//

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <bjson/BjsonDecoder.hpp>

#ifdef WIN32
# include <io.h>
# include <fcntl.h>
#endif /* WIN32 */

#define BUFFER_SIZE 1024

class MyBjsonDecoder : public BjsonDecoder
{
  int _deepIdx;

  //
  // Helper function to pretty format decoded tokens.
  //

  void _deep_printf(const char *fmt, ...)
  {
    va_list args;

    // Print deep spaces first to show tree structure.
    for (int i = 0; i < _deepIdx * 2; i++)
    {
      printf(" ");
    }

    // Print caller specified message.
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");
  }

  // ---------------------------------------------------------------------------
  // Callback functions called when next token was successfuly decoded.
  // We use these functions to tracks what is going on while deciding.
  // ---------------------------------------------------------------------------

  bjson_decoderCallbackResult_t onNull()
  {
    _deep_printf("null");
    return bjson_decoderCallbackResult_Continue;
  }

  bjson_decoderCallbackResult_t onBoolean(int value)
  {
    _deep_printf("boolean (%s)", value ? "true" : "false");
    return bjson_decoderCallbackResult_Continue;
  }

  bjson_decoderCallbackResult_t onInteger(long long value)
  {
    _deep_printf("integer (%lld)", value);
    return bjson_decoderCallbackResult_Continue;
  }

  bjson_decoderCallbackResult_t onDouble(double value)
  {
    _deep_printf("double (%lf)", value);
    return bjson_decoderCallbackResult_Continue;
  }

  bjson_decoderCallbackResult_t onString(const unsigned char *text, size_t textLen)
  {
    _deep_printf("string ('%.*s')", textLen, text);
    return bjson_decoderCallbackResult_Continue;
  }

  bjson_decoderCallbackResult_t onMapKey(const unsigned char *text, size_t textLen)
  {
    _deep_printf("key ('%.*s')", textLen, text);
    return bjson_decoderCallbackResult_Continue;
  }

  bjson_decoderCallbackResult_t onStartMap()
  {
    _deep_printf("{");
    _deepIdx++;
    return bjson_decoderCallbackResult_Continue;
  }

  bjson_decoderCallbackResult_t onEndMap()
  {
    _deepIdx--;
    _deep_printf("}");
    return bjson_decoderCallbackResult_Continue;
  }

  bjson_decoderCallbackResult_t onStartArray()
  {
    _deep_printf("[");
    _deepIdx++;
    return bjson_decoderCallbackResult_Continue;
  }

  bjson_decoderCallbackResult_t onEndArray()
  {
    _deepIdx--;
    _deep_printf("]");
    return bjson_decoderCallbackResult_Continue;
  }

public:

  MyBjsonDecoder() : _deepIdx(0) {}
};


// -----------------------------------------------------------------------------
//                                 Entry point
// -----------------------------------------------------------------------------

int main()
{
  uint8_t bjsonBuffer[BUFFER_SIZE] = {0};

  size_t bytesReaded = 0;

  int goOn = 1;

  //
  // Set stdin to binary mode on windows. We're going to read raw
  // binary buffer (input bjson stream).
  //

  #ifdef WIN32
  _setmode(0, _O_BINARY);
  freopen(NULL, "rb", stdin);
  #endif

  //
  // Create new decoder context.
  //

  MyBjsonDecoder bjsonDecoder;

  //
  // Read BJSON from stdin by <BUFFER_SIZE> chunks.
  //

  while(goOn)
  {
    // Read next BJSON chunk.
    bytesReaded = fread(bjsonBuffer, 1, BUFFER_SIZE, stdin);

    if (bytesReaded > 0)
    {
      // Pass readed chunk to decoder.
      if (bjsonDecoder.parse(bjsonBuffer, bytesReaded) != bjson_status_ok)
      {
        // Data passed, but decode process failed. Don't go on anymore.
        goOn = 0;
      }
    }
    else
    {
      // EOF or error. Don't go on anymore.
      goOn = 0;

      if (!feof(stdin))
      {
        fprintf(stderr, "ERROR: Can't read from input stream.\nError code is: %d", errno);
      }
    }
  }

  //
  // All input data processed. Tell the library, that we passed
  // all expected bytes. This step is needed to detect unterminated
  // BJSON stream e.g. in the middle of array.
  //

  if (bjsonDecoder.complete() != bjson_status_ok)
  {
    //
    // Error - whole buffer processed, but something is wrong while decoding.
    // Check what is going on.
    //

    printf("parse error: %s\n", bjsonDecoder.formatErrorMessage(1));
  }

  return 0;
}
