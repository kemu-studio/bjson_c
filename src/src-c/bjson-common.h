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

#ifndef _BJSON_COMMON_H_
#define _BJSON_COMMON_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Set library version at compilation level.
 */

#define BJSON_MAJOR 1
#define BJSON_MINOR 1
#define BJSON_MICRO 0

#define BJSON_VERSION ((BJSON_MAJOR * 10000) + (BJSON_MINOR * 100) + BJSON_MICRO)

/*
 * Defines.
 */

#if (defined(_WIN32) || defined(WIN32)) && defined(BJSON_SHARED)
#  ifdef BJSON_BUILD
#    define BJSON_API __declspec(dllexport)
#  else
#    define BJSON_API __declspec(dllimport)
#  endif
#else
#  if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 303
#    define BJSON_API __attribute__ ((visibility("default")))
#  else
#    define BJSON_API
#  endif
#endif

/* Maximum depth level for nested array and map containers. */
#define BJSON_MAX_DEPTH 1024

/*
 * Error codes.
 */

typedef enum
{
  /* Success - no error was encountered */
  bjson_status_ok,

  /* A client callback returned zero, stopping the parse */
  bjson_status_canceledByClient,

  /* Error states */
  bjson_status_error_notImplemented,
  bjson_status_error_invalidDataType,
  bjson_status_error_unexpectedEndOfStream,
  bjson_status_error_unhandledDecodeStage,
  bjson_status_error_tooManyNestedContainers,
  bjson_status_error_outOfMemory,
  bjson_status_error_invalidObjectKey,
  bjson_status_error_unclosedMap,
  bjson_status_error_unclosedArray,
  bjson_status_error_keyWithoutValue,
  bjson_status_error_moreDataThanDeclared,
  bjson_status_error_emptyInputPassed,
  bjson_status_error_closeMapButArrayOpen,
  bjson_status_error_closeArrayButMapOpen,
  bjson_status_error_closeMapAtRootLevel,
  bjson_status_error_closeArrayAtRootLevel,
  bjson_status_error_negativeSize
}
bjson_status_t;

/*
 * User defined memory management functions.
 * This struct is used when caller wants to use own malloc/free like
 * functions e.g. for debug purpose.
 */

typedef struct
{
  /*
   * Pointers to memory management functions
   */

  void *(*malloc) (void *ctx, size_t size);
  void  (*free)   (void *ctx, void *ptr);
  void *(*realloc)(void *ctx, void *ptr, size_t newSize);

  /*
   * Caller context
   */

  void *ctx;
}
bjson_memoryFunctions_t;

BJSON_API const char *bjson_getStatusAsText(bjson_status_t statusCode);
BJSON_API const char *bjson_getTokenName(uint8_t tokenId);
BJSON_API const char *bjson_getVersionAsText();
BJSON_API unsigned int bjson_getVersion();

#ifdef __cplusplus
}
#endif

#endif /* _BJSON_COMMON_H_ */
