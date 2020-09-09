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

#include "bjson-encode.h"
#include "bjson-common.h"
#include "bjson-constants.h"
#include "bjson-debug.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define MAX_VALUE_UINT8  0xff
#define MAX_VALUE_UINT16 0xffff
#define MAX_VALUE_UINT32 0xffffffff

#define BJSON_DEFAULT_ARRAY_HEADER_SIZE (sizeof(uint32_t) + 1)

/*
 * ----------------------------------------------------------------------------
 *                        Private structs and typedefs
 * ----------------------------------------------------------------------------
 */

typedef struct bjson_encodeCtx
{
  bjson_status_t statusCode;

  uint8_t *outData;
  size_t outDataIdx;
  size_t outDataCapacity;

  /*
   * User defined functions used as replacement for standard
   * malloc/realloc/free. These callbacks are options. If NULL
   * pased, then default ones are used.
   */

  void *callerCtx;

  bjson_memoryFunctions_t *memoryFunctions;

  /*
   * Track nested arrays/maps state.
   */

  int deepIdx;
  size_t blockIdx[BJSON_MAX_DEPTH + 1];
  uint8_t blockIsMap[BJSON_MAX_DEPTH + 1];
  uint8_t blockMapTurn[BJSON_MAX_DEPTH + 1];
}
bjson_encodeCtx_t;

/*
 * ----------------------------------------------------------------------------
 *                    Support for user defined memory functions
 * ----------------------------------------------------------------------------
 */

static void *bjson_malloc(bjson_encodeCtx_t *ctx, size_t size)
{
  void *rv = NULL;

  if (ctx -> memoryFunctions)
  {
    rv = ctx -> memoryFunctions -> malloc(ctx -> callerCtx, size);
  }
  else
  {
    rv = malloc(size);
  }

  return rv;
}

static void bjson_free(bjson_encodeCtx_t *ctx, void *ptr)
{
  if (ptr)
  {
    if (ctx -> memoryFunctions)
    {
      ctx -> memoryFunctions -> free(ctx -> callerCtx, ptr);
    }
    else
    {
      free(ptr);
    }
  }
}

static void *bjson_realloc(bjson_encodeCtx_t *ctx, void *ptr, size_t newSize)
{
  void *rv = NULL;

  if (ctx -> memoryFunctions)
  {
    rv = ctx -> memoryFunctions -> realloc(ctx -> callerCtx, ptr, newSize);
  }
  else
  {
    rv = realloc(ptr, newSize);
  }

  return rv;
}

/*
 * ----------------------------------------------------------------------------
 *                              Internal helpers
 * ----------------------------------------------------------------------------
 */

static void _setErrorState(bjson_encodeCtx_t *ctx, bjson_status_t statusCode)
{
  BJSON_DEBUG("encoder: set error state (%d): '%s'",
              statusCode, bjson_getStatusAsText(statusCode));

  ctx -> statusCode = statusCode;
}

static int _isOk(bjson_encodeCtx_t *ctx)
{
  return (ctx -> statusCode == bjson_status_ok);
}

static int _isKeyTurn(bjson_encodeCtx_t *ctx)
{
  return ((ctx -> deepIdx > 0) && (ctx -> blockMapTurn[ctx -> deepIdx]));
}

static void _rotateMapTurn(bjson_encodeCtx_t *ctx)
{
  if ((ctx -> deepIdx > 0) && (ctx -> blockIsMap[ctx -> deepIdx]))
  {
    ctx -> blockMapTurn[ctx -> deepIdx] = !ctx -> blockMapTurn[ctx -> deepIdx];
  }
}

static void _setErrorStateIfKeyTurn(bjson_encodeCtx_t *ctx)
{
  if (_isOk(ctx))
  {
    if (_isKeyTurn(ctx))
    {
      _setErrorState(ctx, bjson_status_error_invalidObjectKey);
    }
  }
}

static void _prepareOutDataBuffer(bjson_encodeCtx_t *ctx,
                                  size_t numberOfExtraBytesNeeded)
{
  if (_isOk(ctx) && ctx -> outDataCapacity - ctx -> outDataIdx < numberOfExtraBytesNeeded)
  {
    /*
     * Not enough space - resize buffer.
     */

    size_t newCapacity = MAX(ctx -> outDataCapacity * 2,
                             ctx -> outDataCapacity + numberOfExtraBytesNeeded);

    ctx -> outData         = bjson_realloc(ctx, ctx -> outData, newCapacity);
    ctx -> outDataCapacity = newCapacity;

    if (ctx -> outData)
    {
      BJSON_DEBUG2("encoder: resized outData buffer to [%d] bytes", newCapacity);
    }
    else
    {
      _setErrorState(ctx, bjson_status_error_outOfMemory);
    }
  }
}

static void _putRaw_BLOB(bjson_encodeCtx_t *ctx,
                                   const void *buf, size_t bufSize)
{
  _prepareOutDataBuffer(ctx, bufSize);

  if (_isOk(ctx))
  {
    memcpy(ctx -> outData + ctx -> outDataIdx, buf, bufSize);

    ctx -> outDataIdx += bufSize;
  }
}

static void _putRaw_BYTE(bjson_encodeCtx_t *ctx, uint8_t value)
{
  BJSON_DEBUG3("encoder: going to put byte [%d] at offset [%d]",
               value, ctx -> outDataIdx);

  _prepareOutDataBuffer(ctx, 1);

  if (_isOk(ctx))
  {
    ctx -> outData[ctx -> outDataIdx] = value;
    ctx -> outDataIdx++;
  }
}

static void _putRaw_WORD(bjson_encodeCtx_t *ctx, uint16_t value)
{
  BJSON_DEBUG3("encoder: going to put word [%d] at offset [%d]",
               value, ctx -> outDataIdx);

  _putRaw_BLOB(ctx, &value, sizeof(value));
}

static void _putRaw_DWORD(bjson_encodeCtx_t *ctx, uint32_t value)
{
  BJSON_DEBUG3("encoder: going to put dword [%d] at offset [%d]",
               value, ctx -> outDataIdx);

  _putRaw_BLOB(ctx, &value, sizeof(value));
}

static void _putRaw_QWORD(bjson_encodeCtx_t *ctx, uint64_t value)
{
  BJSON_DEBUG3("encoder: going to put qword [%d] at offset [%d]",
               value, ctx -> outDataIdx);

  _putRaw_BLOB(ctx, &value, sizeof(value));
}

static void _putRaw_FLOAT64(bjson_encodeCtx_t *ctx, double value)
{
  BJSON_DEBUG3("encoder: going to put float64 [%lf] at offset [%d]",
               value, ctx -> outDataIdx);

  _putRaw_BLOB(ctx, &value, sizeof(value));
}

static void _encodeSizedDataType(bjson_encodeCtx_t *ctx,
                                 uint8_t dataTypeBase, uint64_t size)
{
  BJSON_DEBUG3("encoder: going to encode sized data type base [%d], size [%d]",
               dataTypeBase, size);

  if (size < 0)
  {
    _setErrorState(ctx, bjson_status_error_negativeSize);
  }
  else if (size <= MAX_VALUE_UINT8)
  {
    /*
     * Compact to single byte (uint8).
     */

    _putRaw_BYTE(ctx, dataTypeBase | BJSON_DATASIZE_BYTE);
    _putRaw_BYTE(ctx, size);
  }
  else if (size <= MAX_VALUE_UINT16)
  {
    /*
     * Compact to single word (uint16).
     */

    _putRaw_BYTE(ctx, dataTypeBase | BJSON_DATASIZE_WORD);
    _putRaw_WORD(ctx, size);
  }
  else if (size <= MAX_VALUE_UINT32)
  {
    /*
     * Compact to double word (uint32).
     */

    _putRaw_BYTE(ctx, dataTypeBase | BJSON_DATASIZE_DWORD);
    _putRaw_DWORD(ctx, size);
  }
  else
  {
    /*
     * Last try - use quad word (uint64).
     */

    _putRaw_BYTE(ctx, dataTypeBase | BJSON_DATASIZE_QWORD);
    _putRaw_QWORD(ctx, size);
  }
}

static void _enterMapOrArray(bjson_encodeCtx_t *ctx, int isMap)
{
  if (ctx -> deepIdx == BJSON_MAX_DEPTH)
  {
    /* Error - too many nested containers (maps/arrays). */
    _setErrorState(ctx, bjson_status_error_tooManyNestedContainers);
  }
  else
  {
    /*
     * We don't know data size yet, so we reserve room for
     * pesimistic 32-bit size scenario. We'll compact it at the
     * last step if possible.
     */

    _rotateMapTurn(ctx);

    static uint8_t arrayHeaderFiller[] =
    {
      MAX_VALUE_UINT8,
      MAX_VALUE_UINT8,
      MAX_VALUE_UINT8,
      MAX_VALUE_UINT8,
      MAX_VALUE_UINT8
    };

    ctx -> deepIdx++;
    ctx -> blockIsMap[ctx -> deepIdx]   = isMap;
    ctx -> blockIdx[ctx -> deepIdx]     = ctx -> outDataIdx;
    ctx -> blockMapTurn[ctx -> deepIdx] = 0;

    _putRaw_BLOB(ctx, arrayHeaderFiller, sizeof(arrayHeaderFiller));

    BJSON_DEBUG("encoder: entered '%s', deep [%d], dataIdx [%d]",
                isMap ? "map" : "array",
                ctx -> deepIdx,
                ctx -> outDataIdx);
  }
}

static void _leaveMapOrArray(bjson_encodeCtx_t *ctx, int isMap)
{
  if (ctx -> deepIdx < 1)
  {
    /*
     * Error - neither map nor array open.
     */

    if (isMap)
    {
      _setErrorState(ctx, bjson_status_error_closeMapAtRootLevel);
    }
    else
    {
      _setErrorState(ctx, bjson_status_error_closeArrayAtRootLevel);
    }

  }
  else if (ctx -> blockIsMap[ctx -> deepIdx] != isMap)
  {
    /*
     * Error - type mismatch while closing i.e. map closed, but
     * array open or vice versa.
     */

    if (isMap)
    {
      _setErrorState(ctx, bjson_status_error_closeMapButArrayOpen);
    }
    else
    {
      _setErrorState(ctx, bjson_status_error_closeArrayButMapOpen);
    }
  }
  else
  {
    /*
     * Calculate real body size.
     */

    size_t headerIdx  = ctx -> blockIdx[ctx -> deepIdx];
    size_t headerSize = 0;
    size_t bodySize   = ctx -> outDataIdx - headerIdx - BJSON_DEFAULT_ARRAY_HEADER_SIZE;

    BJSON_DEBUG2("encoder: calculated array/map size is [%d] bytes", bodySize);

    /*
     * Fill up header padded at enterXxx() call.
     */

    ctx -> outDataIdx = headerIdx;

    if (ctx -> blockIsMap[ctx -> deepIdx])
    {
      _encodeSizedDataType(ctx, BJSON_DATATYPE_MAP_BASE, bodySize);
    }
    else
    {
      _encodeSizedDataType(ctx, BJSON_DATATYPE_ARRAY_BASE, bodySize);
    }

    /*
     * Move body backward if header is less than 32-bit.
     */

    headerSize = ctx -> outDataIdx - headerIdx;

    if (headerSize < BJSON_DEFAULT_ARRAY_HEADER_SIZE)
    {
      uint8_t *newBody = ctx -> outData + headerIdx + headerSize;
      uint8_t *oldBody = ctx -> outData + headerIdx + BJSON_DEFAULT_ARRAY_HEADER_SIZE;

      memmove(newBody, oldBody, bodySize);
    }

    ctx -> outDataIdx = headerIdx + headerSize + bodySize;
    ctx -> deepIdx--;

    BJSON_DEBUG("encoder: leaved '%s', deep [%d], dataIdx [%d]",
                isMap ? "map" : "array",
                ctx -> deepIdx + 1,
                ctx -> outDataIdx);
  }
}

/*
 * ----------------------------------------------------------------------------
 *                                 Public API
 * ----------------------------------------------------------------------------
 */


/*
 * Create new encoder context. All other bjson_encodeXxx() functions
 * need this context to work.
 *
 * memoryFunctions  - optional struct containing pointers to custom
 *                    malloc/free/realloc functions. Set to NULL if
 *                    not needed (IN/OPT).
 *
 * callerCtx        - optional caller specified context passed to all
 *                    user memory callbacks. Set to NULL if not
 *                    needed (IN/OPT).
 *
 * WARNING! Returned context *MUST* be freed by caller using
 *          bjson_encoderDestroy() function.
 *
 * RETURNS: Pointer to new allocated encoder context if success,
 *          NULL if error.
 */

BJSON_API bjson_encodeCtx_t *bjson_encoderCreate(
                                 bjson_memoryFunctions_t *memoryFunctions,
                                 void *callerCtx)
{
  bjson_encodeCtx_t *ctx = calloc(sizeof(bjson_encodeCtx_t), 1);

  ctx -> memoryFunctions = memoryFunctions;
  ctx -> callerCtx       = callerCtx;

  return ctx;
}

/*
 * Free encoder context.
 *
 * ctx - encoder context created by bjson_encoderCreate() before (IN),
 */

BJSON_API void bjson_encoderDestroy(bjson_encodeCtx_t *ctx)
{
  if (ctx)
  {
    if (ctx -> outData)
    {
      bjson_free(ctx, ctx -> outData);
    }

    free(ctx);
  }
}

/*
 * Get pointer to output buffer containing encoded raw BJSON bytes.
 *
 * TIP#1: This call returns pointer to internal allocated buffer.
 *        Caller *MUST NOT* free it by own.
 *
 * TIP#2: Output buffer is freed when bjson_encoderDestroy() is called.
 *
 * ctx     - encoder context created by bjson_encoderCreate() before (IN),
 * buf     - pointer to internal buffer containing encoded BJSON data (OUT).
 * bufSize - number of bytes stored in returned buf[] buffer (OUT).
 *
 * RETURNS: bjson_status_ok if success,
 *          one of bjson_status_error_xxx codes otherwise.
 */

BJSON_API bjson_status_t bjson_encoderGetResult(bjson_encodeCtx_t *ctx,
                                                void **buf, size_t *bufSize)
{
  if (_isOk(ctx))
  {
    *buf     = ctx -> outData;
    *bufSize = ctx -> outDataIdx;
  }

  return ctx -> statusCode;
}

/*
 * Not implemented.
 */

BJSON_API bjson_status_t bjson_encoderClear(bjson_encodeCtx_t *ctx)
{
  _setErrorState(ctx, bjson_status_error_notImplemented);

  return ctx -> statusCode;
}

/*
 * Not implemented.
 */

BJSON_API bjson_status_t bjson_encoderReset(bjson_encodeCtx_t *ctx,
                                            const char *sepText)
{
  /* Unused parameter */
  (void) sepText;

  _setErrorState(ctx, bjson_status_error_notImplemented);

  return ctx -> statusCode;
}

/*
 * Push null value into output BJSON stream.
 *
 * ctx - encoder context created by bjson_encoderCreate() before (IN),
 *
 * RETURNS: bjson_status_ok if success,
 *          one of bjson_status_error_xxx codes otherwise.
 */

BJSON_API bjson_status_t bjson_encodeNull(bjson_encodeCtx_t *ctx)
{
  _setErrorStateIfKeyTurn(ctx);

  if (_isOk(ctx))
  {
    _putRaw_BYTE(ctx, BJSON_DATATYPE_NULL);
    _rotateMapTurn(ctx);

    BJSON_DEBUG("encoder: encoded null, deep [%d], dataIdx [%d]",
                ctx -> deepIdx, ctx -> outDataIdx);
  }

  return ctx -> statusCode;
}

/*
 * Push boolean value into output BJSON stream.
 *
 * ctx   - encoder context created by bjson_encoderCreate() before (IN).
 * value - set 1 to encode true value, 0 otherwise (IN).
 *
 * RETURNS: bjson_status_ok if success,
 *          one of bjson_status_error_xxx codes otherwise.
 */

BJSON_API bjson_status_t bjson_encodeBool(bjson_encodeCtx_t *ctx, int value)
{
  _setErrorStateIfKeyTurn(ctx);

  if (_isOk(ctx))
  {
    uint8_t dataType = value ? BJSON_DATATYPE_STRICT_TRUE : BJSON_DATATYPE_STRICT_FALSE;

    _putRaw_BYTE(ctx, dataType);
    _rotateMapTurn(ctx);

    BJSON_DEBUG("encoder: encoded bool (%d), deep [%d], dataIdx [%d]",
                value, ctx -> deepIdx, ctx -> outDataIdx);
  }

  return ctx -> statusCode;
}

/*
 * Push integer value into output BJSON stream.
 *
 * ctx   - encoder context created by bjson_encoderCreate() before (IN),
 * value - integer value to be encoded e.g. 1234 (IN).
 *
 * RETURNS: bjson_status_ok if success,
 *          one of bjson_status_error_xxx codes otherwise.
 */

BJSON_API bjson_status_t bjson_encodeInteger(bjson_encodeCtx_t *ctx,
                                             int64_t value)
{
  _setErrorStateIfKeyTurn(ctx);

  if (_isOk(ctx))
  {
    if (value == 0)
    {
      /*
       * Strict integer zero.
       */

      _putRaw_BYTE(ctx, BJSON_DATATYPE_STRICT_INTEGER_ZERO);

      BJSON_DEBUG("encoder: encoded integer zero, deep [%d], dataIdx [%d]",
                  ctx -> deepIdx, ctx -> outDataIdx);
    }
    else if (value == 1)
    {
      /*
       * Strict integer one.
       */

      _putRaw_BYTE(ctx, BJSON_DATATYPE_STRICT_INTEGER_ONE);

      BJSON_DEBUG("encoder: encoded integer one, deep [%d], dataIdx [%d]",
                  ctx -> deepIdx, ctx -> outDataIdx);
    }
    else if (value < 0)
    {
      /*
       * Negative integer.
       */

      _encodeSizedDataType(ctx, BJSON_DATATYPE_NEGATIVE_INTEGER_BASE, -value);

      BJSON_DEBUG("encoder: encoded negative integer (%lld), deep [%d], dataIdx [%d]",
                  value, ctx -> deepIdx, ctx -> outDataIdx);
    }
    else
    {
      /*
       * Positive integer.
       */

      _encodeSizedDataType(ctx, BJSON_DATATYPE_POSITIVE_INTEGER_BASE, value);

      BJSON_DEBUG("encoder: encoded positive integer (%lld), deep [%d], dataIdx [%d]",
                  value, ctx -> deepIdx, ctx -> outDataIdx);
    }

    _rotateMapTurn(ctx);
  }

  return ctx -> statusCode;
}

/*
 * Push double precision number into output BJSON stream.
 *
 * ctx   - encoder context created by bjson_encoderCreate() before (IN).
 * value - number to be encoded e.g. 3.14 (IN).
 *
 * RETURNS: bjson_status_ok if success,
 *          one of bjson_status_error_xxx codes otherwise.
 */

BJSON_API bjson_status_t bjson_encodeDouble(bjson_encodeCtx_t *ctx, double value)
{
  _setErrorStateIfKeyTurn(ctx);

  if (_isOk(ctx))
  {
    _putRaw_BYTE(ctx, BJSON_DATATYPE_FLOAT64);
    _putRaw_FLOAT64(ctx, value);
    _rotateMapTurn(ctx);

    BJSON_DEBUG("encoder: encoded double (%lf), deep [%d], dataIdx [%d]",
                value, ctx -> deepIdx, ctx -> outDataIdx);
  }

  return ctx -> statusCode;
}

/*
 * Not implemented. Reserved for the future use.
 */

BJSON_API bjson_status_t bjson_encodeNumberFromText(bjson_encodeCtx_t *ctx,
                                                    const char *text,
                                                    size_t textLen)
{
  /* Mark unused params */
  (void) text;
  (void) textLen;

  _setErrorState(ctx, bjson_status_error_notImplemented);

  return ctx -> statusCode;
}

/*
 * Push utf8 string into output BJSON stream.
 *
 * ctx     - encoder context created by bjson_encoderCreate() before (IN).
 *
 * text    - buffer containing text to be encoded. Zero byte terminator
 *           is *NOT* neccesery. (IN).
 *
 * textLen - number of bytes stored in text[] buffer *WITHOUT* zero terminator
 *           if exists (IN).
 *
 * RETURNS: bjson_status_ok if success,
 *          one of bjson_status_error_xxx codes otherwise.
 */

BJSON_API bjson_status_t bjson_encodeString(bjson_encodeCtx_t *ctx,
                                            const char *text,
                                            size_t textLen)
{
  if (_isOk(ctx))
  {
    if (textLen == 0)
    {
      /*
       * Special case - empty string.
       */

      _putRaw_BYTE(ctx, BJSON_DATATYPE_EMPTY_STRING);

      BJSON_DEBUG("encoder: encoded empty string, deep [%d], dataIdx [%d]",
                  ctx -> deepIdx, ctx -> outDataIdx);
    }
    else
    {
      /*
       * String header: DATATYPE_STRINGxx <byte-size>.
       */

      _encodeSizedDataType(ctx, BJSON_DATATYPE_STRING_BASE, textLen);

      /*
       * String body (utf8 *WITHOUT* zero terminator).
       */

      BJSON_DEBUG3("encoder: going to put [%d] bytes of utf8 string at offset [%d]",
                   textLen, ctx -> outDataIdx);

      _putRaw_BLOB(ctx, text, textLen);

      /*
       * Log encode event.
       */

      BJSON_DEBUG("encoder: encoded %s '%.*s', deep [%d], dataIdx [%d]",
                  _isKeyTurn(ctx) ? "key" : "string",
                  textLen, text,
                  ctx -> deepIdx,
                  ctx -> outDataIdx);
    }

    _rotateMapTurn(ctx);
  }

  return ctx -> statusCode;
}

/*
 * Push C-style (ASCIIZ, zero terminated) string into output BJSON stream.
 *
 * ctx  - encoder context created by bjson_encoderCreate() before (IN).
 * text - buffer containing null terminated text to be encoded (IN).
 *
 * RETURNS: bjson_status_ok if success,
 *          one of bjson_status_error_xxx codes otherwise.
 */

BJSON_API bjson_status_t bjson_encodeCString(bjson_encodeCtx_t *ctx,
                                             const char *text)
{
  return bjson_encodeString(ctx, text, strlen(text));
}

/*
 * Push arbitrary binary blob into output BJSON stream.
 *
 * ctx      - encoder context created by bjson_encoderCreate() before (IN).
 * blob     - buffer containing data to be encoded (IN).
 * blobSize - number of bytes stored in blob[] buffer (IN).
 *
 * RETURNS: bjson_status_ok if success,
 *          one of bjson_status_error_xxx codes otherwise.
 */

BJSON_API bjson_status_t bjson_encodeBinary(bjson_encodeCtx_t *ctx,
                                            void *blob,
                                            size_t blobSize)
{
  _setErrorStateIfKeyTurn(ctx);

  if (_isOk(ctx))
  {
    /*
     * Binary blob header: DATATYPE_BINARYxx <byte-size>.
     */

    _encodeSizedDataType(ctx, BJSON_DATATYPE_BINARY_BASE, blobSize);

    /*
     * Binary blob data - raw bytes go here.
     */

    BJSON_DEBUG3("encoder: going to put [%d] bytes of binary blob at offset [%d]",
                 blobSize, ctx -> outDataIdx);

    _putRaw_BLOB(ctx, blob, blobSize);
    _rotateMapTurn(ctx);

    BJSON_DEBUG("encoder: encoded [%u] bytes of binary blob, deep [%d], dataIdx [%d]",
                blobSize, ctx -> deepIdx, ctx -> outDataIdx);
  }

  return ctx -> statusCode;
}

/*
 * Begin encoding array container (values list) into output BJSON stream.
 *
 * WARNING! Each bjson_encodeArrayOpen() call *MUST* be followed by
 *          bjson_encodeArrayClose() call.
 *
 * Example (encode [1,2,3]):
 *   bjson_encodeArrayOpen(ctx)
 *     bjson_encodeInteger(ctx, 1)
 *     bjson_encodeInteger(ctx, 2)
 *     bjson_encodeInteger(ctx, 3)
 *   bjson_encodeArrayClose(ctx)
 *
 * ctx - encoder context created by bjson_encoderCreate() before (IN).
 *
 * RETURNS: bjson_status_ok if success,
 *          one of bjson_status_error_xxx codes otherwise.
 */

BJSON_API bjson_status_t bjson_encodeArrayOpen(bjson_encodeCtx_t *ctx)
{
  _setErrorStateIfKeyTurn(ctx);

  if (_isOk(ctx))
  {
    _enterMapOrArray(ctx, 0);
    _rotateMapTurn(ctx);
  }

  return ctx -> statusCode;
}

/*
 * Close array open by bjson_encodeArrayOpen() before.
 *
 * ctx - encoder context created by bjson_encoderCreate() before (IN).
 *
 * RETURNS: bjson_status_ok if success,
 *          one of bjson_status_error_xxx codes otherwise.
 */

BJSON_API bjson_status_t bjson_encodeArrayClose(bjson_encodeCtx_t *ctx)
{
  if (_isOk(ctx))
  {
    _leaveMapOrArray(ctx, 0);
  }

  return ctx -> statusCode;
}

/*
 * Begin encoding map container (key->value list) into output BJSON stream.
 *
 * WARNING! Each bjson_encodeMapOpen() call *MUST* be followed by
 *          bjson_encodeMapClose() call.
 *
 * TIP#1: Use bjson_encodeString() or bjson_encodeCString() to encode keys.
 *
 * Example (encode {'key1': 1, 'key2': 2})
 *   bjson_encodeMapOpen(ctx)
 *     bjson_encodeCString(ctx, "key1")
 *     bjson_encodeInteger(ctx, 1)
 *     bjson_encodeCString(ctx, "key2")
 *     bjson_encodeInteger(ctx, 2)
 *   bjson_encodeMapClose(ctx)
 *
 * ctx - encoder context created by bjson_encoderCreate() before (IN).
 *
 * RETURNS: bjson_status_ok if success,
 *          one of bjson_status_error_xxx codes otherwise.
 */

BJSON_API bjson_status_t bjson_encodeMapOpen(bjson_encodeCtx_t *ctx)
{
  _setErrorStateIfKeyTurn(ctx);

  if (_isOk(ctx))
  {
    _enterMapOrArray(ctx, 1);
    _rotateMapTurn(ctx);
  }

  return ctx -> statusCode;
}

/*
 * Close map open by bjson_encodeMapOpen() before.
 *
 * ctx - encoder context created by bjson_encoderCreate() before (IN).
 *
 * RETURNS: bjson_status_ok if success,
 *          one of bjson_status_error_xxx codes otherwise.
 */

BJSON_API bjson_status_t bjson_encodeMapClose(bjson_encodeCtx_t *ctx)
{
  if (_isOk(ctx))
  {
    _leaveMapOrArray(ctx, 1);
  }

  return ctx -> statusCode;
}

/*
 * Create human readable error message coresponding to current encoder
 * state.
 *
 * ctx    - encoder context created by bjson_encoderCreate() before (IN),
 * vebose - add extra, context specified information if set to 1 (IN).
 *
 * WARNING! Returned pointer *MUST* be freed by caller using
 *          bjson_encoderFreeErrorMessage() function.
 *
 * RETURNS: Pointer to new allocated error message.
 */

BJSON_API char *bjson_encoderFormatErrorMessage(bjson_encodeCtx_t *ctx, int verbose)
{
  /*
   * Possible improvement: add more context related info (verbose mode).
   */

  int   msgCapacity = BJSON_MAX_ERROR_MESSAGE_LENGTH;
  char *msgText     = bjson_malloc(ctx, msgCapacity + 1);

  snprintf(msgText, msgCapacity, "%s", bjson_getStatusAsText(ctx -> statusCode));

  return msgText;
}

/*
 * Free error message allocated by bjson_encoderFormatErrorMessage() before.
 *
 * ctx      - encoder context created by bjson_encoderCreate() before (IN),
 *
 * errorMsg - pointer to error message returned by
 *            bjson_decoderFormatErrorMessage() function (IN).
 */

BJSON_API void bjson_encoderFreeErrorMessage(bjson_encodeCtx_t *ctx, char *errorMsg)
{
  bjson_free(ctx, errorMsg);
}

/*
 * Retrieve current encoder status.
 *
 * RETURNS: bjson_status_ok if encoder is ready to work,
 *          one of bjson_status_error_xxx codes if any error occures.
 */

BJSON_API bjson_status_t bjson_encoderGetStatus(bjson_encodeCtx_t *ctx)
{
  return ctx -> statusCode;
}
