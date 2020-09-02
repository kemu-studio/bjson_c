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
 * This is a basic example showing how to use KEMU BJSON library to encode
 * tokens into bjson stream.
 */

#include <stdio.h>
#include <errno.h>
#include <bjson/bjson-encode.h>

#ifdef WIN32
# include <io.h>
# include <fcntl.h>
#endif /* WIN32 */

int main()
{
  bjson_encodeCtx_t *encodeCtx = NULL;

  void  *bjsonBuffer     = NULL;
  size_t bjsonBufferSize = 0;

  /*
   * Set stdout to binary mode on windows. We're going to print out
   * raw binary buffer (output bjson stream).
   */

  #ifdef WIN32
  _setmode(1, _O_BINARY);
  freopen(NULL, "wb", stdout);
  #endif

  /*
   * Create new encoder context.
   */

  encodeCtx = bjson_encoderCreate(NULL, NULL);

  /*
   * Let's encode some tokens now!
   * We can use bjson_encodeXxx() functions.
   * We're going to encode below JSON document:
   * {
   *   "key1" : ["Text example", 1234, 3.14],
   *   "key2" : true,
   *   "key3" : null
   * }
   */

  bjson_encodeMapOpen(encodeCtx);

    /*
     * "key1" : ["Text example", 1234, 3.14]
     */

    bjson_encodeCString(encodeCtx, "key1");
    bjson_encodeArrayOpen(encodeCtx);
      bjson_encodeCString(encodeCtx, "Text example");
      bjson_encodeInteger(encodeCtx, 1234);
      bjson_encodeDouble(encodeCtx, 3.14d);
    bjson_encodeArrayClose(encodeCtx);

    /*
     * "key2" : true
     */

    bjson_encodeCString(encodeCtx, "key2");
    bjson_encodeBool(encodeCtx, 1);

    /*
     * "key2" : true
     */

    bjson_encodeCString(encodeCtx, "key3");
    bjson_encodeNull(encodeCtx);

  bjson_encodeMapClose(encodeCtx);

  /*
   * BJSON encoded. Now get pointers to created buffer.
   */

  if (bjson_encoderGetResult(encodeCtx,
                             &bjsonBuffer,
                             &bjsonBufferSize) == bjson_status_ok)
  {
    /*
     * Success, put encoded binary buffer to stdout.
     */

    fwrite(bjsonBuffer, bjsonBufferSize, 1, stdout);
  }
  else
  {
    /*
     * Error while encoding. Show what is going on.
     */

    char *errorMsg = bjson_encoderFormatErrorMessage(encodeCtx, 1);

    fprintf(stderr, "%s", errorMsg);

    bjson_encoderFreeErrorMessage(encodeCtx, errorMsg);
  }

  /*
   * Clean up.
   * Free encoder context.
   */

  bjson_encoderDestroy(encodeCtx);

  return 0;
}
