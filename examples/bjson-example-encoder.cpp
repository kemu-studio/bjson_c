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

#include <stdio.h>
#include <errno.h>
#include <bjson/BjsonEncoder.hpp>

#ifdef WIN32
# include <io.h>
# include <fcntl.h>
#endif /* WIN32 */

int main()
{
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
   * Create new encoder object.
   */

  BjsonEncoder bjsonEncoder;

  /*
   * Let's encode some tokens now!
   * We can use bjsonEncoder.encodeXxx() functions.
   * We're going to encode below JSON document:
   * {
   *   "key1" : ["Text example", 1234, 3.14],
   *   "key2" : true,
   *   "key3" : null
   * }
   */

  bjsonEncoder.encodeMapOpen();

    /*
     * "key1" : ["Text example", 1234, 3.14]
     */

    bjsonEncoder.encodeCString("key1");
    bjsonEncoder.encodeArrayOpen();
      bjsonEncoder.encodeCString("Text example");
      bjsonEncoder.encodeInteger(1234);
      bjsonEncoder.encodeDouble(3.14d);
    bjsonEncoder.encodeArrayClose();

    /*
     * "key2" : true
     */

    bjsonEncoder.encodeCString("key2");
    bjsonEncoder.encodeBool(1);

    /*
     * "key2" : true
     */

    bjsonEncoder.encodeCString("key3");
    bjsonEncoder.encodeNull();

  bjsonEncoder.encodeMapClose();

  /*
   * BJSON encoded. Now get pointers to created buffer.
   */

  if (bjsonEncoder.getResult(&bjsonBuffer,
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

    fprintf(stderr, "%s", bjsonEncoder.formatErrorMessage(1).c_str());
  }

  return 0;
}
