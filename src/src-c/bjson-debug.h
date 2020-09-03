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

#ifndef _BJSON_DEBUG_H_
#define _BJSON_DEBUG_H_

#define BJSON_DEBUG_LEVEL 0
#define BJSON_DEBUG_DUMP_BUFFERS 0

/*
 * Dump buffers to log.
 */

#if BJSON_DEBUG_DUMP_BUFFERS > 0
  #define BJSON_DEBUG_DUMP(_buf_, _bufSize_) bjson_debug_dumpBuffer(_buf_, _bufSize_)
#else
  #define BJSON_DEBUG_DUMP(_buf_, _bufSize_)
#endif

/*
 * Debug level 3.
 */

#if BJSON_DEBUG_LEVEL >= 3
  #define BJSON_DEBUG3(_fmt_, ...) fprintf(stderr, "bjson3: " _fmt_ "\n", __VA_ARGS__)
#else
  #define BJSON_DEBUG3(_fmt_, ...)
#endif

/*
 * Debug level 2.
 */

#if BJSON_DEBUG_LEVEL >= 2
  #define BJSON_DEBUG2(_fmt_, ...) fprintf(stderr, "bjson2: " _fmt_ "\n", __VA_ARGS__)
#else
  #define BJSON_DEBUG2(_fmt_, ...)
#endif

/*
 * Debug level 1.
 */

#if BJSON_DEBUG_LEVEL >= 1
  #define BJSON_DEBUG(_fmt_, ...)  fprintf(stderr, "bjson1: " _fmt_ "\n", __VA_ARGS__)
  #define BJSON_DEBUG1(_fmt_, ...) fprintf(stderr, "bjson1: " _fmt_ "\n", __VA_ARGS__)
#else
  #define BJSON_DEBUG(_fmt_, ...)
  #define BJSON_DEBU1(_fmt_, ...)
#endif

#endif /* _BJSON_DEBUG_H_ */
