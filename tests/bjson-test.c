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

/* ----------------------------------------------------------------------------
 *                                    Includes
 * ---------------------------------------------------------------------------*/

#include <bjson/bjson-decode.h>
#include <bjson/bjson-encode.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <inttypes.h>

#ifdef WIN32
# include <io.h>
# include <fcntl.h>
#endif /* WIN32 */

/* ----------------------------------------------------------------------------
 *                           Defines and helper macros.
 * ---------------------------------------------------------------------------*/

#define DEFAULT_BUFFER_SIZE (1024*4)
#define TEST_MODE_DECODE 0
#define TEST_MODE_ENCODE 1

#define TEST_CTX(vptr) ((bjsonTestMemoryContext_t *) (vptr))
#define DIE(...) {fprintf(stderr, __VA_ARGS__); exit(-1);}

/* ----------------------------------------------------------------------------
 *                               Global variables.
 * ---------------------------------------------------------------------------*/

static int g_bjson_testMode = TEST_MODE_DECODE;

static bjson_decodeCtx_t *g_decodeCtx = NULL;
static bjson_encodeCtx_t *g_encodeCtx = NULL;

/* ----------------------------------------------------------------------------
 *                             Structs and typedefs.
 * ---------------------------------------------------------------------------*/

typedef struct
{
  unsigned int numFrees;
  unsigned int numMallocs;
}
bjsonTestMemoryContext_t;

/* ----------------------------------------------------------------------------
 * Custom memory management functions to count alloc/free callcs for debug
 * purpose. These functions are called by bjson as replacement for standard
 * malloc/free/realloc functions.
 * ---------------------------------------------------------------------------*/

static void bjsonTestFree(void *ctx, void *ptr)
{
  assert(ptr);

  TEST_CTX(ctx) -> numFrees++;

  free(ptr);
}

static void *bjsonTestMalloc(void *ctx, size_t sz)
{
  assert(sz != 0);

  void *rv = malloc(sz);

  if (rv)
  {
    TEST_CTX(ctx) -> numMallocs++;
  }

  return rv;
}

static void *bjsonTestRealloc(void *ctx, void *ptr, size_t sz)
{
  void *rv = realloc(ptr, sz);

  if (ptr == NULL && rv)
  {
    assert(sz != 0);

    TEST_CTX(ctx) -> numMallocs++;
  }
  else if (sz == 0)
  {
    TEST_CTX(ctx) -> numFrees++;
  }

  return rv;
}

/* ----------------------------------------------------------------------------
 * Callback functions called when next token was successfuly decoded.
 * We use these functions to tracks what is going on while deciding.
 * ---------------------------------------------------------------------------*/

static int test_bjson_null(void *ctx)
{
  if (g_bjson_testMode == TEST_MODE_ENCODE)
  {
    bjson_encodeNull(g_encodeCtx);
  }
  else
  {
    printf("null\n");
  }

  return 1;
}

static int test_bjson_boolean(void *ctx, int value)
{
  if (g_bjson_testMode == TEST_MODE_ENCODE)
  {
    bjson_encodeBool(g_encodeCtx, value);
  }
  else
  {
    printf("bool: %s\n", value ? "true" : "false");
  }

  return 1;
}

static int test_bjson_integer(void *ctx, int64_t value)
{
  if (g_bjson_testMode == TEST_MODE_ENCODE)
  {
    bjson_encodeInteger(g_encodeCtx, value);
  }
  else
  {
    printf("integer: %" PRId64 "\n", value);
  }

  return 1;
}

static int test_bjson_double(void *ctx, double value)
{
  if (g_bjson_testMode == TEST_MODE_ENCODE)
  {
    bjson_encodeDouble(g_encodeCtx, value);
  }
  else
  {
    /*
     * %g gives 1e+009 on MinGW, but 1e+09 on Linux.
     * We normalize exponent digits by own to make output platform
     * independend.
     */

    char valueText[32] = {0};
    char *expText      = NULL;

    int valueTextCapacity = sizeof(valueText) - 1;

    snprintf(valueText, valueTextCapacity, "%.15g", value);

    expText = strchr(valueText, 'e');

    if (expText)
    {
      /*
       * Exponent notation detected. Reprint exponent part
       * once again, but using fixed number of digits.
       */

      expText++;

      int baseTextSize    = expText - valueText;
      int expTextCapacity = valueTextCapacity - baseTextSize;
      snprintf(expText, expTextCapacity, "%+04d", atoi(expText));
    }

    printf("double: %s\n", valueText);
  }

  return 1;
}

static int test_bjson_string(void *ctx,
                             const unsigned char *text,
                             size_t textLen)
{
  if (g_bjson_testMode == TEST_MODE_ENCODE)
  {
    bjson_encodeString(g_encodeCtx, text, textLen);
  }
  else
  {
    printf("string: '");
    fwrite(text, 1, textLen, stdout);
    printf("'\n");
  }

  return 1;
}

static int test_bjson_map_key(void *ctx,
                             const unsigned char *text,
                             size_t textLen)
{
  if (g_bjson_testMode == TEST_MODE_ENCODE)
  {
    bjson_encodeString(g_encodeCtx, text, textLen);
  }
  else
  {
    printf("key: '");
    fwrite(text, 1, textLen, stdout);
    printf("'\n");
  }

  return 1;
}

static int test_bjson_start_map(void *ctx)
{
  if (g_bjson_testMode == TEST_MODE_ENCODE)
  {
    bjson_encodeMapOpen(g_encodeCtx);
  }
  else
  {
    printf("map open '{'\n");
  }

  return 1;
}

static int test_bjson_end_map(void *ctx)
{
  if (g_bjson_testMode == TEST_MODE_ENCODE)
  {
    bjson_encodeMapClose(g_encodeCtx);
  }
  else
  {
    printf("map close '}'\n");
  }

  return 1;
}

static int test_bjson_start_array(void *ctx)
{
  if (g_bjson_testMode == TEST_MODE_ENCODE)
  {
    bjson_encodeArrayOpen(g_encodeCtx);
  }
  else
  {
    printf("array open '['\n");
  }

  return 1;
}

static int test_bjson_end_array(void *ctx)
{
  if (g_bjson_testMode == TEST_MODE_ENCODE)
  {
    bjson_encodeArrayClose(g_encodeCtx);
  }
  else
  {
    printf("array close ']'\n");
  }

  return 1;
}

/* ----------------------------------------------------------------------------
 *                                Entry point.
 * ---------------------------------------------------------------------------*/

int main(int argc, char ** argv)
{
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
   * Prepare custom memory management functions. We use them to track
   * memory allocations.
   */

  bjson_memoryFunctions_t memoryFunctions =
  {
    bjsonTestMalloc,
    bjsonTestFree,
    bjsonTestRealloc
  };

  bjsonTestMemoryContext_t memCtx = {0, 0};

  /* Input BJSON file or stdin if not specified. */
  const char *fileName = NULL;
  FILE *file = NULL;

  /* Working buffer to store next BJSON chunk. */
  unsigned char *buf = NULL;
  size_t bufSize     = DEFAULT_BUFFER_SIZE;
  size_t bytesReaded = 0;

  /* BJSON decoder specified. */
  bjson_status_t statusCode = bjson_status_ok;

  /* Runtime helpers. */
  int goOn = 1;
  int i = 0;

  /*
   * Parse command line parameters.
   */

  for (i = 1; i < argc; i++)
  {
    /*
     * -x like: treat as option
     */

    if (argv[i][0] == '-')
    {
      if (strcmp(argv[i], "-b") == 0)
      {
        /*
         * -b <buffer-size>
         */

        if (i == argc - 1)
        {
          DIE("ERROR: Missing value after -b parameter.\n");
        }
        else
        {
          bufSize = atoi(argv[i+1]);

          i++;
        }
      }
      else if (strcmp(argv[i], "--decode") == 0)
      {
        g_bjson_testMode = TEST_MODE_DECODE;
      }
      else if (strcmp(argv[i], "--encode") == 0)
      {
        g_bjson_testMode = TEST_MODE_ENCODE;
      }
      else
      {
        /*
         * Unknown -x parameter.
         */

        DIE("ERROR: Unknown parameter [%s].\n", argv[i]);
      }
    }
    else
    {
      /*
       * Default way - treat as filename
       */

      fileName = argv[i];
    }
  }

  /*
   * Set binary mode on windows on input file to avoid interpreting
   * zero byte as end of file.
   */

  #ifdef WIN32
  _setmode(0, _O_BINARY);
  freopen(NULL, "rb", stdin);
  #endif

  /*
   * Choose input source (file vs stdin).
   */

  if (fileName)
  {
    /* File name passed in command line parameters - use it. */
    file = fopen(fileName, "rb");
  }
  else
  {
    /* No file name passed - read from stdin instead. */
    file     = stdin;
    fileName = "[stdin]";
  }

  /*
   * Allocate working buffer.
   */

  buf = malloc(bufSize);

  if (buf == NULL)
  {
    DIE("ERROR: Can't allocate working buffer.\n");
  }

  /*
   * Create new decoder/encoder contexts.
   */

  g_decodeCtx = bjson_decoderCreate(&callbacks, &memoryFunctions, &memCtx);

  if (g_bjson_testMode == TEST_MODE_ENCODE)
  {
    g_encodeCtx = bjson_encoderCreate(&memoryFunctions, &memCtx);
  }

  /*
   * Pass whole input BJSON file to decoder.
   */

  while(goOn)
  {
    /* Read next BJSON chunk. */
    bytesReaded = fread(buf, 1, bufSize, file);

    if (bytesReaded > 0)
    {
      /* Pass readed chunk to decoder. */
      statusCode = bjson_decoderParse(g_decodeCtx, buf, bytesReaded);

      if (statusCode != bjson_status_ok)
      {
        /* Data passed, but decode process failed. Don't go on anymore. */
        goOn = 0;
      }
    }
    else
    {
      /* EOF or error. Don't go on anymore. */
      goOn = 0;

      if (!feof(file))
      {
        DIE("ERROR: Can't read '%s'.\nError code is: %d", fileName, errno);
      }
    }
  }

  statusCode = bjson_decoderComplete(g_decodeCtx);

  if (statusCode != bjson_status_ok)
  {
    char *errorMsg = bjson_decoderFormatErrorMessage(g_decodeCtx, 0);

    printf("parse error: %s\n", errorMsg);

    bjson_decoderFreeErrorMessage(g_decodeCtx, errorMsg);
  }

  /*
   * Output re-encoded BJSON for encode test.
   */

  if (g_bjson_testMode == TEST_MODE_ENCODE)
  {
    void *output      = NULL;
    size_t outputSize = 0;

    bjson_encoderGetResult(g_encodeCtx, &output, &outputSize);

    if (outputSize > 0)
    {
      #ifdef WIN32
      _setmode(1, _O_BINARY);
      freopen(NULL, "wb", stdout);
      #endif

      fwrite(output, 1, outputSize, stdout);
    }
  }

  /*
   * Clean up.
   */

  bjson_decoderDestroy(g_decodeCtx);
  bjson_encoderDestroy(g_encodeCtx);

  if (fileName)
  {
    fclose(file);
  }

  if (buf)
  {
    free(buf);
  }

  /*
   * Print memory statistics collected by custom memory callbacks.
   */

  if (g_bjson_testMode == TEST_MODE_DECODE)
  {
    printf("memory leaks:\t%u\n", memCtx.numMallocs - memCtx.numFrees);
  }

  fflush(stderr);
  fflush(stdout);

  return 0;
}
