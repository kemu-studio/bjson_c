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

#include "bjson-decode.h"
#include "bjson-common.h"
#include "bjson-constants.h"
#include "bjson-debug.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define BJSON_DATATYPE_SIZE_MASK (0x3U)
#define BJSON_DATATYPE_BASE_MASK (~0x3U)

/*
 * Helper macro to pass decoded token via caller callback.
 * We do below steps here:
 * 1. Check if given callback is defined by caller,
 * 2. If yes, then call it (parameters depend on callback),
 * 3. Check the return code got from callback:
 * - bjson_decoderCallbackResult_Continue: then we go to the next token and
 *  continue decoding,
 * - bjson_decoderCallbackResult_Abort: stop the decoder (don't go on to the
 *  next token),
 * - bjson_decoderCallbackResult_StepOver: Not implemented,
 * - bjson_decoderCallbackResult_StepOutside: Not implemented.
 */

#define PASS_TOKEN0(_ctx_, _cb_)                                                            \
  {                                                                                         \
    if (_ctx_->callbacks->_cb_)                                                             \
    {                                                                                       \
      if (_ctx_->callbacks->_cb_(_ctx_->callerCtx) != bjson_decoderCallbackResult_Continue) \
      {                                                                                     \
        _setErrorState(_ctx_, bjson_status_canceledByClient);                               \
      }                                                                                     \
    }                                                                                       \
  }

#define PASS_TOKEN(_ctx_, _cb_, ...)                                                                     \
  {                                                                                                      \
    if (_ctx_->callbacks->_cb_)                                                                          \
    {                                                                                                    \
      if (_ctx_->callbacks->_cb_(_ctx_->callerCtx, __VA_ARGS__) != bjson_decoderCallbackResult_Continue) \
      {                                                                                                  \
        _setErrorState(_ctx_, bjson_status_canceledByClient);                                            \
      }                                                                                                  \
    }                                                                                                    \
  }

/*
 * ----------------------------------------------------------------------------
 *                        Private structs and typedefs
 * ----------------------------------------------------------------------------
 */

/*
 * Stage I:   <TYPE>                 (obgligatory)
 * Stage II:  <BODYSIZE_OR_IMMVALUE> (optional)
 * Stage III: <BODY>                 (optional)
 */

typedef enum
{
  bjson_decodeStage_dataType,
  bjson_decodeStage_bodySizeOrImmValue,
  bjson_decodeStage_stringOrBinaryBody,
  bjson_decodeStage_error,
} bjson_decodeStage_t;

typedef struct bjson_decodeCtx
{
  /*
   * Current decoder status. We store it to track error code.
   */

  bjson_status_t statusCode;

  /*
   * Currect decoder stage. We store data used to decode currently
   * processed token (integer, string, array open etc.) here.
   * When current token is parsed completely state and dataXxx
   * fields are reset to initial states and then we go to next token.
   * These fields are related to *current token* only.
   */

  bjson_decodeStage_t stage;

  size_t dataIdx;
  uint8_t dataType;
  uint8_t dataTypeBase;
  uint8_t dataTypeSize;

  union
  {
    size_t   bodySize;
    uint64_t valueInteger;
    float    valueFloat;
    double   valueDouble;
  }
  bodySizeOrImmValue;

  /*
   * Track nested arrays/maps state. We need it to detect when to
   * close map/array correctly without recursive code.
   * These fields are common for *whole decode process*, it means
   * they are used along whole tokens in the stream.
   */

  size_t  blockEndIdx[BJSON_MAX_DEPTH + 1];
  uint8_t blockMapTurn[BJSON_MAX_DEPTH + 1];
  uint8_t blockType[BJSON_MAX_DEPTH + 1];

  int deepIdx;

  /*
   * Bytes missing to finish currect decode stage.
   * We know if there is no at least <bytesMissing> bytes available, then
   * it's not possible to go on and we still need to wait for more data.
   *
   * This fields are needed to handle fragmented data e.g. in the middle
   * of string or event in the middle of double number. We store fragmented
   * data and join them inside our temporary cache buffer and pass to caller
   * when whole token arrived e.g. when we got complete string.
   */

  void *cache;
  size_t cacheCapacity;
  size_t cacheIdx;
  size_t cacheBytesMissing;

  /*
   * User defined callbacks, executed each time when one of bjson
   * token (integer, double, string etc.) was successfuly decoded.
   */

  bjson_decoderCallbacks_t *callbacks;

  /*
   * User defined functions used as replacement for standard
   * malloc/realloc/free. These callbacks are options. If NULL
   * pased, then default ones are used.
   */

  bjson_memoryFunctions_t *memoryFunctions;

  /*
   * Optional user defined callback passed to all callbacks above if
   * set.
   */

  void *callerCtx;
} bjson_decodeCtx_t;

/*
 * ----------------------------------------------------------------------------
 *                              Internal helpers
 * ----------------------------------------------------------------------------
 */

static int _isOk(bjson_decodeCtx_t *ctx)
{
  return (ctx->statusCode == bjson_status_ok);
}

static void _setErrorState(bjson_decodeCtx_t *ctx, bjson_status_t statusCode)
{
  BJSON_DEBUG("decoder: set decoder error state (%d): '%s'",
              statusCode, bjson_getStatusAsText(statusCode));

  ctx->statusCode = statusCode;
  ctx->stage      = bjson_decodeStage_error;
}

static void _enterMapOrArray(bjson_decodeCtx_t *ctx)
{
  if (ctx->deepIdx == BJSON_MAX_DEPTH)
  {
    /* Error - too many nested containers (maps/arrays). */
    _setErrorState(ctx, bjson_status_error_tooManyNestedContainers);
  }
  else
  {
    /*
     * There is still enough room on containers stack - go on one
     * level deeper.
     */

    ctx->deepIdx++;
    ctx->blockType[ctx->deepIdx]    = ctx->dataTypeBase;
    ctx->blockEndIdx[ctx->deepIdx]  = ctx->dataIdx + ctx->bodySizeOrImmValue.bodySize;
    ctx->blockMapTurn[ctx->deepIdx] = 0;
    ctx->stage = bjson_decodeStage_dataType;

    if (ctx->dataTypeBase == BJSON_DATATYPE_ARRAY_BASE)
    {
      PASS_TOKEN0(ctx, bjson_start_array);

      BJSON_DEBUG("decoder: entered array type [%d], deep [%d], bodyIdx [%u], endIdx [%u]",
                  ctx->dataType,
                  ctx->deepIdx,
                  ctx->dataIdx,
                  ctx->blockEndIdx[ctx->deepIdx]);
    }
    else
    {
      PASS_TOKEN0(ctx, bjson_start_map);

      BJSON_DEBUG("decoder: entered map type [%d], deep [%d], bodyIdx [%u], endIdx [%u]",
                  ctx->dataType,
                  ctx->deepIdx,
                  ctx->dataIdx,
                  ctx->blockEndIdx[ctx->deepIdx]);
    }
  }
}

static int _isKeyTurn(bjson_decodeCtx_t *ctx)
{
  return ((ctx->deepIdx > 0) && (ctx->blockMapTurn[ctx->deepIdx]));
}

static void _rotateMapTurn(bjson_decodeCtx_t *ctx)
{
  if ((ctx->deepIdx > 0) && (ctx->blockType[ctx->deepIdx] == BJSON_DATATYPE_MAP_BASE))
  {
    ctx->blockMapTurn[ctx->deepIdx] = !ctx->blockMapTurn[ctx->deepIdx];
  }
}

static void _tryLeaveMapOrArray(bjson_decodeCtx_t *ctx)
{
  int goOn = 1;

  /*
   * Go on until there is any map/array to close or error.
   */

  while (goOn)
  {
    if (ctx->deepIdx > 0)
    {
      size_t nearestBlockEndIdx = ctx->blockEndIdx[ctx->deepIdx];

      if (ctx->dataIdx > nearestBlockEndIdx)
      {
        /*
         * Error - inconsistent BJSON detected. Container body is out of
         * size declared in header.
         *
         * Example: array8 with size 1, but integer32 passed as child item.
         * ARRAY8, 1, POSITIVE_INTEGER32, 0x78, 0x56, 0x34, 0x12
         *                                ^ Out of bounds here
         */

        _setErrorState(ctx, bjson_status_error_moreDataThanDeclared);

        goOn = 0;
      }
      else if (nearestBlockEndIdx == ctx->dataIdx)
      {
        /*
         * End of map/array exactly matches current position.
         * This is an ordinary way how container should be closed.
         * We're going to close nearest one and pop it from containers stack.
         */

        if (ctx->blockType[ctx->deepIdx] == BJSON_DATATYPE_ARRAY_BASE)
        {
          /*
           * Close nearsest array.
           */

          if (ctx->callbacks->bjson_end_array)
          {
            ctx->callbacks->bjson_end_array(ctx->callerCtx);
          }

          BJSON_DEBUG("decoder: leaved array type [%d], deep [%d], dataIdx [%u]",
                      ctx->dataType, ctx->deepIdx, ctx->dataIdx);
        }
        else
        {
          if (_isKeyTurn(ctx))
          {
            /*
             * Error - map closed in the middle of {key,value} pair i.e.
             * key without value detected.
             * Example: {'key1': 1, 'key2'}
             *                            ^
             *                           Missing value here
             */

            _setErrorState(ctx, bjson_status_error_keyWithoutValue);

            goOn = 0;
          }
          else
          {
            /*
             * Close nearest map.
             */

            if (ctx->callbacks->bjson_end_map)
            {
              ctx->callbacks->bjson_end_map(ctx->callerCtx);
            }

            BJSON_DEBUG("decoder: leaved map type [%d], deep [%d], dataIdx [%u]",
                        ctx->dataType, ctx->deepIdx, ctx->dataIdx);
          }
        }

        ctx->deepIdx--;
      }
      else
      {
        goOn = 0;
      }
    }
    else
    {
      goOn = 0;
    }
  }
}

/*
 * ----------------------------------------------------------------------------
 *               Internal wrappers to user defined callbacks
 *
 * We go to one of these functions each time when next token (integer, string
 * etc.) is decoded and ready to pass to caller.
 * ----------------------------------------------------------------------------
 */

static void _passNull(bjson_decodeCtx_t *ctx)
{
  PASS_TOKEN0(ctx, bjson_null);
}

static void _passBoolean(bjson_decodeCtx_t *ctx, int value)
{
  PASS_TOKEN(ctx, bjson_boolean, value);
}

static void _passInteger(bjson_decodeCtx_t *ctx, int64_t value)
{
  PASS_TOKEN(ctx, bjson_integer, value);
}

static void _passDouble(bjson_decodeCtx_t *ctx, double value)
{
  PASS_TOKEN(ctx, bjson_double, value);
}

static void _passString(bjson_decodeCtx_t *ctx,
                        const unsigned char *buf,
                        size_t bufLen)
{
  if (_isKeyTurn(ctx))
  {
    PASS_TOKEN(ctx, bjson_map_key, buf, bufLen);
  }
  else
  {
    PASS_TOKEN(ctx, bjson_string, buf, bufLen);
  }
}

static void _passBinary(bjson_decodeCtx_t *ctx, const void *buf, size_t bufLen)
{
  PASS_TOKEN(ctx, bjson_binary, buf, bufLen);
}

/*
 * ----------------------------------------------------------------------------
 *                    Support for user defined memory functions
 * ----------------------------------------------------------------------------
 */

static void *bjson_malloc(bjson_decodeCtx_t *ctx, size_t size)
{
  void *rv = NULL;

  if (ctx->memoryFunctions)
  {
    rv = ctx->memoryFunctions->malloc(ctx->callerCtx, size);
  }
  else
  {
    rv = malloc(size);
  }

  return rv;
}

static void bjson_free(bjson_decodeCtx_t *ctx, void *ptr)
{
  if (ctx->memoryFunctions)
  {
    ctx->memoryFunctions->free(ctx->callerCtx, ptr);
  }
  else
  {
    free(ptr);
  }
}

static void *bjson_realloc(bjson_decodeCtx_t *ctx, void *ptr, size_t newSize)
{
  void *rv = NULL;

  /* Realloc is called only if we need more space than current allocated one.
   * Should be always greater to zero.
   * Zero size should never happen on production. */
  assert(newSize > 0);

  if (ctx->memoryFunctions)
  {
    rv = ctx->memoryFunctions->realloc(ctx->callerCtx, ptr, newSize);
  }
  else
  {
    rv = realloc(ptr, newSize);
  }

  return rv;
}

/*
 * ----------------------------------------------------------------------------
 *                         Internal cache functions
 *
 * Used to handle fragmented input stream. If we have too less data to finish
 * current decoder stage, then we pause decode process and accumulate (fetch)
 * incoming bytes into internal cache buffer. When number of accumulated data
 * is sufficient to execute next decoder stage, then we pass cached buffer to
 * decoder and restart ordinary decode process.
 * ----------------------------------------------------------------------------
 */

static void _cacheBegin(bjson_decodeCtx_t *ctx, size_t bytesNeeded)
{
  void *newCache = NULL;

  if (ctx->cache == NULL)
  {
    /*
     * Cache used first time. There is no buffer allocated yet.
     * Allocate new one.
     */

    newCache = bjson_malloc(ctx, bytesNeeded);

    if (newCache)
    {
      ctx->cache         = newCache;
      ctx->cacheCapacity = bytesNeeded;
      BJSON_DEBUG("decoder: allocated new cache buffer with [%u] bytes", bytesNeeded);
    }
  }
  else if (ctx->cacheCapacity < bytesNeeded)
  {
    /*
     * Cache buffer already allocated, but it's too small to handle
     * all missing bytes.
     */

    newCache = bjson_realloc(ctx, ctx->cache, bytesNeeded);

    if (newCache)
    {
      ctx->cache         = newCache;
      ctx->cacheCapacity = bytesNeeded;

      BJSON_DEBUG("decoder: increased cache buffer to [%u] bytes", bytesNeeded);
    }
  }
  else
  {
    /*
     * Nothing to do, reuse existing cache buffer.
     */

    newCache = ctx->cache;
  }

  ctx->cacheBytesMissing = bytesNeeded;
  ctx->cacheIdx          = 0;

  if (newCache)
  {
    BJSON_DEBUG("decoder: cache started, waiting for [%u] bytes", bytesNeeded);
  }
  else
  {
    _setErrorState(ctx, bjson_status_error_outOfMemory);
  }
}

static void _cacheFetch(bjson_decodeCtx_t *ctx,
                        uint8_t **inData,
                        size_t *inDataSize)
{
  if (_isOk(ctx) && ctx->cache)
  {
    size_t bytesToLoad = MIN(ctx->cacheBytesMissing, *inDataSize);

    if (bytesToLoad > 0)
    {
      /* Avoid false clang-tidy warning:
       * error: Use of memory after it is freed [clang-analyzer-unix.Malloc,-warnings-as-errors]
       * memcpy(ctx->cache + ctx->cacheIdx, *inData, bytesToLoad);
       * ^
       *
       * NOLINTNEXTLINE */
      memcpy(ctx->cache + ctx->cacheIdx, *inData, bytesToLoad);

      ctx->cacheIdx          += bytesToLoad;
      ctx->cacheBytesMissing -= bytesToLoad;

      *inDataSize -= bytesToLoad;
      *inData     += bytesToLoad;

      BJSON_DEBUG("decoder: fetched [%u] bytes to cache, [%u] still needed",
                  bytesToLoad, ctx->cacheBytesMissing);
    }
  }
}

/*
 * ----------------------------------------------------------------------------
 *                                 Public API
 * ----------------------------------------------------------------------------
 */

/*
 * Pass next BJSON chunk to decoder.
 *
 * ctx        - decoder context created by bjson_decoderCreate() before (IN),
 * inData     - buffer containing BJSON data to decode (IN),
 * inDataSize - number of bytes stored inside inData buffer (IN).
 *
 * RETURNS: bjson_status_ok if success,
 *          one of bjson_status_error_xxx codes otherwise.
 */

bjson_status_t bjson_decoderParse(bjson_decodeCtx_t *ctx,
                                  const void *inDataRaw,
                                  size_t inDataSize)
{
  uint8_t *inData = (uint8_t *)inDataRaw;

  /*
   * Try finish fetching missing bytes to complete fragmented token first.
   * This scenario occurs, when last stage could not be finished
   * due to incomplete input data. Check are we ready to finish that
   * pending stage this time.
   */

  if ((ctx->stage != bjson_decodeStage_error) && (ctx->cacheBytesMissing > 0))
  {
    /*
     * Fetch missing bytes to cache.
     */

    _cacheFetch(ctx, &inData, &inDataSize);

    if (ctx->cacheBytesMissing == 0)
    {
      /*
       * All missing bytes loaded. Pass them to decoder and restart
       * decode process as ususal.
       */

      BJSON_DEBUG("decoder: %s", "cache completed, going to restart decode");
      BJSON_DEBUG_DUMP(ctx->cache, ctx->cacheIdx);
      bjson_decoderParse(ctx, ctx->cache, ctx->cacheIdx);
    }
  }

  /*
   * Go on until all input bytes consumed or error.
   * We always process all input buffer no matter is it partially or not.
   */

  while ((inDataSize > 0) && (ctx->stage != bjson_decodeStage_error))
  {
    /*
     * Dispatch current stage if possible.
     */

    switch (ctx->stage)
    {
      case bjson_decodeStage_dataType:
      {
        /*
         * Stage I: Read and decode single byte data type.
         * All next steps are optional and depend on data type decoded at
         * this stage.
         */

        ctx->dataType = inData[0];
        ctx->dataIdx++;

        inData++;
        inDataSize--;

        /*
         * Check key/value consistent if we're inside map.
         * Only string keys are allowed.
         */

        if (_isKeyTurn(ctx) &&
            ((ctx->dataType & BJSON_DATATYPE_BASE_MASK) != BJSON_DATATYPE_STRING_BASE) &&
            (ctx->dataType != BJSON_DATATYPE_EMPTY_STRING))
        {
          _setErrorState(ctx, bjson_status_error_invalidObjectKey);

          break;
        }

        /*
         * Dispatch datatype. We try primitive types first.
         * All primitive types has no additional data, so they fit into single
         * byte and this is the last stage.
         */

        switch (ctx->dataType)
        {
          /*
           * Basic primitives (single byte).
           */

          case BJSON_DATATYPE_NULL:          {_passNull(ctx); break;}
          case BJSON_DATATYPE_ZERO_OR_FALSE: {_passInteger(ctx, 0); break;}
          case BJSON_DATATYPE_ONE_OR_TRUE:   {_passInteger(ctx, 1); break;}
          case BJSON_DATATYPE_EMPTY_STRING:  {_passString(ctx, NULL, 0); break;}

          /*
           * Strict primitives (single byte).
           */

          case BJSON_DATATYPE_STRICT_FALSE:        {_passBoolean(ctx, 0); break;}
          case BJSON_DATATYPE_STRICT_TRUE:         {_passBoolean(ctx, 1); break;}
          case BJSON_DATATYPE_STRICT_INTEGER_ZERO: {_passInteger(ctx, 0); break;}
          case BJSON_DATATYPE_STRICT_INTEGER_ONE:  {_passInteger(ctx, 1); break;}

          default:
          {
            /*
             * Non-primitive types (multi bytes).
             * There is more than first stage needed.
             * Split data type into base and size parts.
             * We store data type size at two less significant bits.
             */

            ctx->dataTypeBase = ctx->dataType & BJSON_DATATYPE_BASE_MASK;
            ctx->dataTypeSize = 1U << (ctx->dataType & BJSON_DATATYPE_SIZE_MASK);

            /*
             * Verify is data type correct.
             */

            switch (ctx->dataTypeBase)
            {
              case BJSON_DATATYPE_POSITIVE_INTEGER_BASE:
              case BJSON_DATATYPE_NEGATIVE_INTEGER_BASE:
              case BJSON_DATATYPE_FLOAT_BASE:
              case BJSON_DATATYPE_STRING_BASE:
              case BJSON_DATATYPE_BINARY_BASE:
              case BJSON_DATATYPE_ARRAY_BASE:
              case BJSON_DATATYPE_MAP_BASE:
              {
                /*
                 * Now, we have decoded data type and we know we need <dataTypeSize>
                 * bytes storing immediate value (integer/float) or body size
                 * (array/map/string).
                 */

                ctx->stage = bjson_decodeStage_bodySizeOrImmValue;

                break;
              }

              default:
              {
                /* Error - unknown data type. */
                _setErrorState(ctx, bjson_status_error_invalidDataType);
              }
            }
          }
        }

        break;
      }

      case bjson_decodeStage_bodySizeOrImmValue:
      {
        /*
         * Stage II: Decode bodySize (string/map/array) or immediate
         * value (integer/float).
         */

        if (inDataSize < ctx->dataTypeSize)
        {
          /*
           * Fragmented input detected at stage II (bodySizeOrImmValue).
           * Start fetching incoming data in the working cache and
           * stop decode process until complete portion of data
           * arrived.
           */

          BJSON_DEBUG("decoder: %s", "fragmented buffer detected on stage II"
                      " (bodySizeOrImmValue), going to start cache");

          _cacheBegin(ctx, ctx->dataTypeSize);
          _cacheFetch(ctx, &inData, &inDataSize);
        }
        else
        {
          /*
           * We have all data needed to finish stage. Go on.
           */

          ctx->bodySizeOrImmValue.valueInteger = 0;

          memcpy(&(ctx->bodySizeOrImmValue), inData, ctx->dataTypeSize);

          ctx->dataIdx += ctx->dataTypeSize;
          inData       += ctx->dataTypeSize;
          inDataSize   -= ctx->dataTypeSize;

          switch (ctx->dataTypeBase)
          {
            /*
             * Positive_integerxx (8/16/32/64).
             */

            case BJSON_DATATYPE_POSITIVE_INTEGER_BASE:
            {
              BJSON_DEBUG("decoder: decoded positive integer%d [%lld]",
                          ctx->dataTypeSize * 8,
                          ctx->bodySizeOrImmValue.valueInteger);

              _passInteger(ctx, ctx->bodySizeOrImmValue.valueInteger);

              ctx->stage = bjson_decodeStage_dataType;

              break;
            }

            /*
             * Negative_integerxx (8/16/32/64).
             */

            case BJSON_DATATYPE_NEGATIVE_INTEGER_BASE:
            {
              BJSON_DEBUG("decoder: decoded negative integer%d [%lld]",
                          ctx->dataTypeSize * 8,
                          -ctx->bodySizeOrImmValue.valueInteger);

              _passInteger(ctx, -ctx->bodySizeOrImmValue.valueInteger);

              ctx->stage = bjson_decodeStage_dataType;

              break;
            }

            /*
             * Floating point number.
             */

            case BJSON_DATATYPE_FLOAT_BASE:
            {
              switch (ctx->dataType)
              {
                case BJSON_DATATYPE_FLOAT32:
                {
                  /*
                   * Float32 (single precision number).
                   */

                  BJSON_DEBUG("decoder: decoded float32 [%f]",
                              ctx->bodySizeOrImmValue.valueFloat);

                  _passDouble(ctx, ctx->bodySizeOrImmValue.valueFloat);

                  break;
                }

                case BJSON_DATATYPE_FLOAT64:
                {
                  /*
                   * Float64 (double precision number)
                   */

                  BJSON_DEBUG("decoder: decoded float64 [%lf]",
                              ctx->bodySizeOrImmValue.valueDouble);

                  _passDouble(ctx, ctx->bodySizeOrImmValue.valueDouble);

                  break;
                }
              }

              ctx->stage = bjson_decodeStage_dataType;

              break;
            }

            /*
             * Stringxx (8/16/32/64).
             */

            case BJSON_DATATYPE_STRING_BASE:
            {
              ctx->stage = bjson_decodeStage_stringOrBinaryBody;

              break;
            }

            /*
             * Arrayxx and mapxx.
             */

            case BJSON_DATATYPE_ARRAY_BASE:
            case BJSON_DATATYPE_MAP_BASE:
            {
              _enterMapOrArray(ctx);

              break;
            }
          }
        }

        break;
      }

      case bjson_decodeStage_stringOrBinaryBody:
      {
        /*
         * Stage III (body): Decode string/binary body.
         */

        if (inDataSize < ctx->bodySizeOrImmValue.bodySize)
        {
          /*
           * Fragmented input detected at stage III (body).
           * Start fetching incoming data to the temporary cache and
           * freeze decode process until complete body will be collected.
           */

          BJSON_DEBUG("decoder: %s", "fragmented buffer detected on stage III (body), "
                      "going to start cache");

          _cacheBegin(ctx, ctx->bodySizeOrImmValue.bodySize);
          _cacheFetch(ctx, &inData, &inDataSize);
        }
        else
        {
          /*
           * We have all data needed to decode string/binary body. Go on.
           */

          if (ctx->dataTypeBase == BJSON_DATATYPE_STRING_BASE)
          {
            _passString(ctx, inData, ctx->bodySizeOrImmValue.bodySize);
          }
          else
          {
            _passBinary(ctx, inData, ctx->bodySizeOrImmValue.bodySize);
          }

          ctx->stage = bjson_decodeStage_dataType;

          ctx->dataIdx += ctx->bodySizeOrImmValue.bodySize;
          inData       += ctx->bodySizeOrImmValue.bodySize;
          inDataSize   -= ctx->bodySizeOrImmValue.bodySize;
        }

        break;
      }

      case bjson_decodeStage_error:
      {
        break;
      }

      default:
      {
        /*
         * Fatal error - unhandled decode stage. Should never happen in
         * production code.
         */

        BJSON_DEBUG("decoder: FATAL: Unhandled decode stage [%d]\n", ctx->stage);

        _setErrorState(ctx, bjson_status_error_unhandledDecodeStage);
      }
    }

    /*
     * Is whole token decoded?
     */

    if (ctx->stage == bjson_decodeStage_dataType)
    {
      /*
       * Is it last item in current conainer?
       * If yes, then leave it.
       */

      _tryLeaveMapOrArray(ctx);

      /*
       * Rotate key/value turn if we're inside map container.
       * We want key,value,key,value... serie.
       */

      _rotateMapTurn(ctx);
    }
  }

  return ctx->statusCode;
}

/*
 * Tell decoder, that the last BJSON chunk was passed.
 * This call is needed to detect scenario when input BJSON stream is
 * finished incorrectly e.g. in the middle of the array.
 *
 * ctx - decoder context created by bjson_decoderCreate() before (IN),
 *
 * RETURNS: bjson_status_ok if whole BJSON document was decoded successffuly,
 *          one of bjson_status_error_xxx codes otherwise.
 */

BJSON_API bjson_status_t bjson_decoderComplete(bjson_decodeCtx_t *ctx)
{
  if (_isOk(ctx))
  {
    /*
     * There was no error at last parse() call.
     * Go on and check did input stream finish correctly.
     */

    if (ctx->dataIdx == 0)
    {
      /*
       * Error - empty input detected. There was no any data passed
       * to the parser
       */

      _setErrorState(ctx, bjson_status_error_emptyInputPassed);
    }
    else if ((ctx->stage != bjson_decodeStage_dataType) ||
             (ctx->cacheBytesMissing > 0))
    {
      /*
       * Error - stream finished in the middle of token
       * or stream finished in the middle of fragmented buffer.
       */

      _setErrorState(ctx, bjson_status_error_unexpectedEndOfStream);
    }
    else if (ctx->deepIdx > 0)
    {
      if (ctx->blockType[ctx->deepIdx] == BJSON_DATATYPE_MAP_BASE)
      {
        /*
         * Error - unclosed map at the end of stream.
         */

        _setErrorState(ctx, bjson_status_error_unclosedMap);
      }
      else
      {
        /*
         * Error - unclosed array at the end of stream.
         */

        _setErrorState(ctx, bjson_status_error_unclosedArray);
      }
    }
  }

  return ctx->statusCode;
}

/*
 * Create new decoder context. All other bjson_decodeXxx() functions
 * need this context to work.
 *
 * decoderCallbacks - struct containing pointer to callback functions
 *                    called when next token (integer, string etc.)
 *                    was sucessfully decoded (IN).
 *
 * memoryFunctions  - optional struct containing pointers to custom
 *                    malloc/free/realloc functions. Set to NULL if
 *                    not needed (IN/OPT).
 *
 * callerCtx        - optional caller specified context passed to all
 *                    user callbacks (both decoder and memory). Set
 *                    to NULL if not needed (IN/OPT).
 *
 * WARNING! Returned context *MUST* be freed by caller using
 *          bjson_decoderDestroy() function.
 *
 * RETURNS: Pointer to new allocated decoder context if success,
 *          NULL if error.
 */

BJSON_API bjson_decodeCtx_t *bjson_decoderCreate(
                      bjson_decoderCallbacks_t *decoderCallbacks,
                      bjson_memoryFunctions_t *memoryFunctions,
                      void *callerCtx)
{
  bjson_decodeCtx_t *ctx = calloc(sizeof(bjson_decodeCtx_t), 1);

  ctx->callbacks       = decoderCallbacks;
  ctx->memoryFunctions = memoryFunctions;
  ctx->callerCtx       = callerCtx;

  return ctx;
}

/*
 * Free decoder context.
 *
 * ctx - decoder context created by bjson_decoderCreate() before (IN),
 */

BJSON_API void bjson_decoderDestroy(bjson_decodeCtx_t *ctx)
{
  if (ctx)
  {
    if (ctx->cache)
    {
      bjson_free(ctx, ctx->cache);
    }

    free(ctx);
  }
}

/*
 * Create human readable error message coresponding to current decoder
 * state.
 *
 * ctx    - decoder context created by bjson_decoderCreate() before (IN),
 * vebose - add extra, context specified information if set to 1 (IN).
 *
 * WARNING! Returned pointer *MUST* be freed by caller using
 *          bjson_decoderFreeErrorMessage() function.
 *
 * RETURNS: Pointer to new allocated error message.
 */

BJSON_API char *bjson_decoderFormatErrorMessage(bjson_decodeCtx_t *ctx, int verbose)
{
  /*
   * Possible improvement: add more context related info.
   */

  size_t msgCapacity = BJSON_MAX_ERROR_MESSAGE_LENGTH;
  char *msgText      = bjson_malloc(ctx, msgCapacity + 1);

  if (verbose)
  {
    /*
     * Format verbose message.
     */
    snprintf(msgText, msgCapacity, "%s near offset %zu (last token is '%s')",
             bjson_getStatusAsText(ctx->statusCode),
             ctx->dataIdx,
             bjson_getTokenName(ctx->dataType));
  }
  else
  {
    /*
     * Non-verbose mode - simply put error status.
     */

    snprintf(msgText, msgCapacity, "%s", bjson_getStatusAsText(ctx->statusCode));
  }

  return msgText;
}

/*
 * Free error message allocated by bjson_decoderFormatErrorMessage() before.
 *
 * ctx      - decoder context created by bjson_decoderCreate() before (IN),
 *
 * errorMsg - pointer to error message returned by
 *            bjson_decoderFormatErrorMessage() function (IN).
 *
 * RETURNS: Pointer to new allocated error message.
 */

BJSON_API void bjson_decoderFreeErrorMessage(bjson_decodeCtx_t *ctx, char *errorMsg)
{
  bjson_free(ctx, errorMsg);
}
