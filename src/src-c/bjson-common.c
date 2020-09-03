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

#include <stdio.h>
#include "bjson-common.h"
#include "bjson-constants.h"

BJSON_API const char *bjson_getStatusAsText(bjson_status_t statusCode)
{
  struct
  {
    bjson_status_t statusCode;
    const char *statusText;
  }
  knownStatusCodes[] =
  {
    {bjson_status_ok,                            "ok"},
    {bjson_status_canceledByClient,              "decode canceled via callback return value"},
    {bjson_status_error_notImplemented,          "not implemented"},
    {bjson_status_error_invalidDataType,         "invalid data type"},
    {bjson_status_error_unexpectedEndOfStream,   "unexpected end of stream"},
    {bjson_status_error_unhandledDecodeStage,    "unhandled decode stage"},
    {bjson_status_error_tooManyNestedContainers, "too many nested containers"},
    {bjson_status_error_outOfMemory,             "out of memory"},
    {bjson_status_error_invalidObjectKey,        "invalid object key"},
    {bjson_status_error_unclosedMap,             "unclosed map"},
    {bjson_status_error_unclosedArray,           "unclosed array"},
    {bjson_status_error_keyWithoutValue,         "missing value after object key"},
    {bjson_status_error_moreDataThanDeclared,    "more data than declared"},
    {bjson_status_error_emptyInputPassed,        "empty input passed"},
    {bjson_status_error_closeMapButArrayOpen,    "going to close map but array open"},
    {bjson_status_error_closeArrayButMapOpen,    "going to close array but map open"},
    {bjson_status_error_closeMapAtRootLevel,     "going to close map at root level"},
    {bjson_status_error_closeArrayAtRootLevel,   "going to close array at root level"},
    {bjson_status_error_negativeSize,            "going to encode negative size value"},

    /* Array terminator. */
    {0, NULL}
  };

  const char *rv = "internal error";

  int i;

  for (i = 0; knownStatusCodes[i].statusText; i++)
  {
    if (knownStatusCodes[i].statusCode == statusCode)
    {
      /* Status code found. Stop searching here. */
      rv = knownStatusCodes[i].statusText;
      break;
    }
  }

  return rv;
}

BJSON_API const char *bjson_getTokenName(uint8_t tokenId)
{
  struct
  {
    uint8_t id;
    const char *name;
  }
  knownTokens[] =
  {
    {BJSON_DATATYPE_NULL               , "null"},
    {BJSON_DATATYPE_ZERO_OR_FALSE      , "zero_or_false"},
    {BJSON_DATATYPE_EMPTY_STRING       , "empty_string"},
    {BJSON_DATATYPE_ONE_OR_TRUE        , "one_or_true"},

    {BJSON_DATATYPE_POSITIVE_INTEGER8  , "positive_integer8"},
    {BJSON_DATATYPE_POSITIVE_INTEGER16 , "positive_integer16"},
    {BJSON_DATATYPE_POSITIVE_INTEGER32 , "positive_integer32"},
    {BJSON_DATATYPE_POSITIVE_INTEGER64 , "positive_integer64"},

    {BJSON_DATATYPE_NEGATIVE_INTEGER8  , "negative_integer8"},
    {BJSON_DATATYPE_NEGATIVE_INTEGER16 , "negative_integer16"},
    {BJSON_DATATYPE_NEGATIVE_INTEGER32 , "negative_integer32"},
    {BJSON_DATATYPE_NEGATIVE_INTEGER64 , "negative_integer64"},

    {BJSON_DATATYPE_FLOAT32_OBSOLETE   , "obsolete_float32"},
    {BJSON_DATATYPE_FLOAT64_OBSOLETE   , "obsolete_float64"},
    {BJSON_DATATYPE_FLOAT32            , "float32"},
    {BJSON_DATATYPE_FLOAT64            , "float64"},

    {BJSON_DATATYPE_STRING8            , "string8"},
    {BJSON_DATATYPE_STRING16           , "string16"},
    {BJSON_DATATYPE_STRING32           , "string32"},
    {BJSON_DATATYPE_STRING64           , "string64"},

    {BJSON_DATATYPE_BINARY8            , "binary8"},
    {BJSON_DATATYPE_BINARY16           , "binary16"},
    {BJSON_DATATYPE_BINARY32           , "binary32"},
    {BJSON_DATATYPE_BINARY64           , "binary64"},

    {BJSON_DATATYPE_ARRAY8             , "array8"},
    {BJSON_DATATYPE_ARRAY16            , "array16"},
    {BJSON_DATATYPE_ARRAY32            , "array32"},
    {BJSON_DATATYPE_ARRAY64            , "array64"},

    {BJSON_DATATYPE_MAP8               , "map8"},
    {BJSON_DATATYPE_MAP16              , "map16"},
    {BJSON_DATATYPE_MAP32              , "map32"},
    {BJSON_DATATYPE_MAP64              , "map64"},

    {BJSON_DATATYPE_STRICT_FALSE       , "strict_false"},
    {BJSON_DATATYPE_STRICT_TRUE        , "strict_true"},
    {BJSON_DATATYPE_STRICT_INTEGER_ZERO, "strict_integer_zero"},
    {BJSON_DATATYPE_STRICT_INTEGER_ONE , "strict_integer_one"},

    /* Array terminator. */
    {0, NULL}
  };

  const char *rv = "unknown";

  int i;

  for (i = 0; knownTokens[i].name; i++)
  {
    if (knownTokens[i].id == tokenId)
    {
      /* Status code found. Stop searching here. */
      rv = knownTokens[i].name;
      break;
    }
  }

  return rv;
}

BJSON_API const char *bjson_getVersionAsText()
{
  static char versionText[32] = {0};

  if (versionText[0] == 0)
  {
    /*
     * First time call - render version to static buffer.
     */

    snprintf(versionText, sizeof(versionText) - 1,
             "%d.%d.%d", BJSON_MAJOR, BJSON_MINOR, BJSON_MICRO);
  }

  return versionText;
}

BJSON_API unsigned int bjson_getVersion()
{
  return BJSON_VERSION;
}
