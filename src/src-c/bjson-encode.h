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

#ifndef _BJSON_ENCODE_H_
#define _BJSON_ENCODE_H_

#include <stddef.h>
#include "bjson-common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Structures and data types.
 */

typedef struct bjson_encodeCtx bjson_encodeCtx_t;

/*
 * Functions to create/destroy encoder context.
 * All other functions need this context to work.
 *
 * TIP#1: Each call to bjson_encoderCreate() MUSTS be followed by
 *        bjson_encoderDestroy() call.
 *
 * TIP#2: Basic usage looks like:
 *
 *        ctx = bjson_encoderCreate(...)
 *          bjson_encodeXxx(ctx, ...)
 *          bjson_encodeXxx(ctx, ...)
 *          bjson_encodeXxx(ctx, ...)
 *          ...
 *        bjson_encoderDestroy(ctx)
 */

BJSON_API bjson_encodeCtx_t *bjson_encoderCreate(
                                 bjson_memoryFunctions_t *memoryFunctions,
                                 void *callerCtx);

BJSON_API void bjson_encoderDestroy(bjson_encodeCtx_t *encodeCtx);

/*
 * Function to retrive pointer to output buffer containing encoded BJSON
 * bytes.
 *
 * TIP#1: This call returns pointer to internal allocated buffer.
 *        Caller *MUST NOT* free it by own.
 *
 * TIP#2: Output buffer is freed when bjson_encoderDestroy() is called.
 */

BJSON_API bjson_status_t bjson_encoderGetResult(bjson_encodeCtx_t *ctx,
                                                void **buf, size_t *bufSize);

/*
 * Function to manage internal encoder state.
 * These functions are helpful to reuse the same encoder context over
 * many JSON documents.
 */

BJSON_API bjson_status_t bjson_encoderClear(bjson_encodeCtx_t *ctx);

BJSON_API bjson_status_t bjson_encoderReset(bjson_encodeCtx_t *ctx,
                                            const char *sepText);

/*
 * bjson_encodeXxx() functions to encode variety tokens into
 * output bjson stream.
 */

BJSON_API bjson_status_t bjson_encodeInteger(bjson_encodeCtx_t *ctx,
                                             long long int value);

BJSON_API bjson_status_t bjson_encodeDouble(bjson_encodeCtx_t *ctx,
                                            double value);

BJSON_API bjson_status_t bjson_encodeNull(bjson_encodeCtx_t *ctx);
BJSON_API bjson_status_t bjson_encodeBool(bjson_encodeCtx_t *ctx, int value);
BJSON_API bjson_status_t bjson_encodeMapOpen(bjson_encodeCtx_t *ctx);
BJSON_API bjson_status_t bjson_encodeMapClose(bjson_encodeCtx_t *ctx);
BJSON_API bjson_status_t bjson_encodeArrayOpen(bjson_encodeCtx_t *ctx);
BJSON_API bjson_status_t bjson_encodeArrayClose(bjson_encodeCtx_t *ctx);

BJSON_API bjson_status_t bjson_encodeNumberFromText(bjson_encodeCtx_t *ctx,
                                                    const char *text,
                                                    size_t textLen);

BJSON_API bjson_status_t bjson_encodeString(bjson_encodeCtx_t *ctx,
                                            const char *text,
                                            size_t textLen);

BJSON_API bjson_status_t bjson_encodeCString(bjson_encodeCtx_t *ctx,
                                             const char *text);

BJSON_API bjson_status_t bjson_encodeBinary(bjson_encodeCtx_t *ctx,
                                            void *blob,
                                            size_t blobSize);

/*
 * Error handling.
 *
 * TIP: Eeach call to bjson_encoderFormatErrorMessage() *MUST* be
 *      followed by bjson_encoderFreeErrorMessage() call.
 */

BJSON_API bjson_status_t bjson_encoderGetStatus(bjson_encodeCtx_t *ctx);

BJSON_API char *
  bjson_encoderFormatErrorMessage(bjson_encodeCtx_t *ctx, int verbose);

BJSON_API void
  bjson_encoderFreeErrorMessage(bjson_encodeCtx_t *ctx, char *errorMsg);

#ifdef __cplusplus
}
#endif

#endif /* _BJSON_ENCODE_H_ */
