/*
 * Copyright (c) 2017 by Kemu Studio (visit ke.mu)
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

/*
 * This is a basic example showing how to decode input BJSON stream into
 * list of tokens.
 */

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <inttypes.h>
#include <bjson/bjson-decode.h>

#ifdef WIN32
# include <io.h>
# include <fcntl.h>
#endif /* WIN32 */

#define BUFFER_SIZE 1024

static int g_deepIdx = 0;

/*
 * Helper function to pretty format decoded tokens.
 */

static void deep_printf(const char *fmt, ...)
{
  int i;

  va_list args;

  /*
   * Print deep spaces first to show tree structure.
   */

  for (i = 0; i < g_deepIdx * 2; i++)
  {
    printf(" ");
  }

  /*
   * Print caller specified message.
   */

  va_start(args, fmt);

  vprintf(fmt, args);

  va_end(args);

  printf("\n");
}

/* ----------------------------------------------------------------------------
 * Callback functions called when next token was successfuly decoded.
 * We use these functions to tracks what is going on while deciding.
 * ---------------------------------------------------------------------------*/

static bjson_decoderCallbackResult_t test_bjson_null(void *ctx)
{
  deep_printf("null");

  return bjson_decoderCallbackResult_Continue;
}

static bjson_decoderCallbackResult_t test_bjson_boolean(void *ctx, int value)
{
  deep_printf("boolean (%s)", value ? "true" : "false");

  return bjson_decoderCallbackResult_Continue;
}

static bjson_decoderCallbackResult_t test_bjson_integer(void *ctx, int64_t value)
{
  deep_printf("integer (%" PRId64 ")", value);

  return bjson_decoderCallbackResult_Continue;
}

static bjson_decoderCallbackResult_t test_bjson_double(void *ctx, double value)
{
  deep_printf("double (%lf)", value);

  return bjson_decoderCallbackResult_Continue;
}

static bjson_decoderCallbackResult_t test_bjson_string(void *ctx,
                             const unsigned char *text,
                             size_t textLen)
{
  deep_printf("string ('%.*s')", textLen, text);

  return bjson_decoderCallbackResult_Continue;
}

static bjson_decoderCallbackResult_t test_bjson_map_key(void *ctx,
                             const unsigned char *text,
                             size_t textLen)
{
  deep_printf("key ('%.*s')", textLen, text);

  return bjson_decoderCallbackResult_Continue;
}

static bjson_decoderCallbackResult_t test_bjson_start_map(void *ctx)
{
  deep_printf("{");
  g_deepIdx++;
  return bjson_decoderCallbackResult_Continue;
}

static bjson_decoderCallbackResult_t test_bjson_end_map(void *ctx)
{
  g_deepIdx--;
  deep_printf("}");
  return bjson_decoderCallbackResult_Continue;
}

static bjson_decoderCallbackResult_t test_bjson_start_array(void *ctx)
{
  deep_printf("[");
  g_deepIdx++;
  return bjson_decoderCallbackResult_Continue;
}

static bjson_decoderCallbackResult_t test_bjson_end_array(void *ctx)
{
  g_deepIdx--;
  deep_printf("]");
  return bjson_decoderCallbackResult_Continue;
}

/* ----------------------------------------------------------------------------
 *                                 Entry point
 * ---------------------------------------------------------------------------*/

int main()
{
  bjson_decodeCtx_t *decodeCtx = NULL;

  uint8_t bjsonBuffer[BUFFER_SIZE] = {0};

  size_t bytesReaded = 0;

  int goOn = 1;

  /*
   * Prepare callbacks called when next token decoded from BJSON stream.
   * We use these callbacks to monitor what is going on while decoding.
   */

  bjson_decoderCallbacks_t callbacks =
  {
    test_bjson_null,
    test_bjson_boolean,
    test_bjson_integer,
    test_bjson_double,
    NULL,
    test_bjson_string,
    test_bjson_start_map,
    test_bjson_map_key,
    test_bjson_end_map,
    test_bjson_start_array,
    test_bjson_end_array
  };

  /*
   * Set stdin to binary mode on windows. We're going to read raw
   * binary buffer (input bjson stream).
   */

  #ifdef WIN32
  _setmode(0, _O_BINARY);
  freopen(NULL, "rb", stdin);
  #endif

  /*
   * Create new decoder context.
   */

  decodeCtx = bjson_decoderCreate(&callbacks, NULL, NULL);

  /*
   * Read BJSON from stdin by <BUFFER_SIZE> chunks.
   */

  while(goOn)
  {
    /*
     * Read next BJSON chunk.
     */

    bytesReaded = fread(bjsonBuffer, 1, BUFFER_SIZE, stdin);

    if (bytesReaded > 0)
    {
      /*
       * Pass readed chunk to decoder.
       */

      if (bjson_decoderParse(decodeCtx,
                             bjsonBuffer,
                             bytesReaded) != bjson_status_ok)
      {
        /*
         * Data passed, but decode process failed. Don't go on anymore.
         */

        goOn = 0;
      }
    }
    else
    {
      /*
       * EOF or error. Don't go on anymore.
       */

      goOn = 0;

      if (!feof(stdin))
      {
        fprintf(stderr, "ERROR: Can't read from input stream.\nError code is: %d", errno);
      }
    }
  }

  /*
   * All input data processed. Tell the library, that we passed
   * while expected bytes. This step is needed to detect unterminated
   * BJSON stream e.g. in the middle of array.
   */

  if (bjson_decoderComplete(decodeCtx) != bjson_status_ok)
  {
    /*
     * Error - whole buffer processed, but something wrong while decoding.
     * Check what is going on.
     */

    char *errorMsg = bjson_decoderFormatErrorMessage(decodeCtx, 1);

    printf("parse error: %s\n", errorMsg);

    bjson_decoderFreeErrorMessage(decodeCtx, errorMsg);
  }

  /*
   * Clean up.
   * Free decoder context.
   */

  bjson_decoderDestroy(decodeCtx);

  return 0;
}
