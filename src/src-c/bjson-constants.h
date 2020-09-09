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

#ifndef _BJSON_CONSTANTS_H_
#define _BJSON_CONSTANTS_H_

/*
 * datasize postfixes used to create final datatype by combaining
 * DATATYPE_XXX_BASE | DATASIZE_XXX parts.
 * For example code for 32 bit positive integer can calculated with below
 * formula:
 * DATATYPE_POSITIVE_INTEGER32 = DATATYPE_POSITIVE_INTEGER_BASE | DATASIZE_DWORD
 */

#define BJSON_DATASIZE_BYTE  0U
#define BJSON_DATASIZE_WORD  1U
#define BJSON_DATASIZE_DWORD 2U
#define BJSON_DATASIZE_QWORD 3U

/*
 * primitive values:
 * There are "zero" values, one byte sized:
 */

#define BJSON_DATATYPE_NULL          0U /* null */
#define BJSON_DATATYPE_ZERO_OR_FALSE 1U /* numeric zero, or boolean false */
#define BJSON_DATATYPE_EMPTY_STRING  2U /* empty string */
#define BJSON_DATATYPE_ONE_OR_TRUE   3U /* boolean true (may be also a numeric one) */

/*
 * positive_integer:
 */

#define BJSON_DATATYPE_POSITIVE_INTEGER_BASE 4U
#define BJSON_DATATYPE_POSITIVE_INTEGER8     4U
#define BJSON_DATATYPE_POSITIVE_INTEGER16    5U
#define BJSON_DATATYPE_POSITIVE_INTEGER32    6U
#define BJSON_DATATYPE_POSITIVE_INTEGER64    7U

/*
 * negative_integer:
 * they are in positive form, not mod2
 */

#define BJSON_DATATYPE_NEGATIVE_INTEGER_BASE  8U
#define BJSON_DATATYPE_NEGATIVE_INTEGER8      8U
#define BJSON_DATATYPE_NEGATIVE_INTEGER16     9U
#define BJSON_DATATYPE_NEGATIVE_INTEGER32     10U
#define BJSON_DATATYPE_NEGATIVE_INTEGER64     11U

/*
 * float:
 */

#define BJSON_DATATYPE_FLOAT_BASE       12U
#define BJSON_DATATYPE_FLOAT32_OBSOLETE 12U /* Obsolete: don't use in new code */
#define BJSON_DATATYPE_FLOAT64_OBSOLETE 13U /* Obsolete: don't use in new code */
#define BJSON_DATATYPE_FLOAT32          14U
#define BJSON_DATATYPE_FLOAT64          15U

/*
 * utf8_string:
 * default coding is utf-8
 * the string MUST NOT have null-termination code
 * string cannot have any "zero" bytes to avoid null-termination
 * finishing the string before its real length
 */

#define BJSON_DATATYPE_STRING_BASE 16U /* 16, size[uint8], utf8_data[size*byte] - a short string up to 255 bytes */
#define BJSON_DATATYPE_STRING8     16U /* 16, size[uint8], utf8_data[size*byte] - a short string up to 255 bytes */
#define BJSON_DATATYPE_STRING16    17U /* 17, size[uint16], utf8_data[size*byte] - a string of up to 64k bytes */
#define BJSON_DATATYPE_STRING32    18U /* 18, size[uint32], utf8_data[size*byte] - a long string, 64K to 4GB */
#define BJSON_DATATYPE_STRING64    19U /* 19, size[uint64], utf8_data[size*byte] - a very long string, which probably wont be even used for now */

/*
 * binary:
 * binary data of specified length.
 * This is not fully JSON transcodable, as the JSON has no native support for binary data.
 */

#define BJSON_DATATYPE_BINARY_BASE 20U /* 20, size[uint8], binary_data[size*byte] */
#define BJSON_DATATYPE_BINARY8     20U /* 20, size[uint8], binary_data[size*byte] */
#define BJSON_DATATYPE_BINARY16    21U /* 21, size[uint16], binary_data[size*byte] */
#define BJSON_DATATYPE_BINARY32    22U /* 22, size[uint32], binary_data[size*byte] */
#define BJSON_DATATYPE_BINARY64    23U /* 23, size[uint64], binary_data[size*byte] */

/*
 * array:
 * in JSON represented as array [item0, item1, item2, ...]
 */

#define BJSON_DATATYPE_ARRAY_BASE 32U /* 32, size[uint8], item0, item1, item2, ...  */
#define BJSON_DATATYPE_ARRAY8     32U /* 32, size[uint8], item0, item1, item2, ...  */
#define BJSON_DATATYPE_ARRAY16    33U /* 33, size[uint16], item0, item1, item2, ... */
#define BJSON_DATATYPE_ARRAY32    34U /* 34, size[uint32], item0, item1, item2, ... */
#define BJSON_DATATYPE_ARRAY64    35U /* 35, size[uint64], item0, item1, item2, ... */

/*
 * map of key -> value:
 * in JSON represented as object {key0:value0, key1:value1, ...}
 * For JSON compatibility keys shall be utf8_string.
 * However implementation may ignore that (use any other type as keys,
 * even mixing types) if the JSON-compatibility is not a requirement.
 * Keys should be unique.
 */

#define BJSON_DATATYPE_MAP_BASE 36U /* 36, size[uint8], key0, value0, key1, value1, ... */
#define BJSON_DATATYPE_MAP8     36U /* 36, size[uint8], key0, value0, key1, value1, ... */
#define BJSON_DATATYPE_MAP16    37U /* 37, size[uint16], key0, value0, key1, value1, ...*/
#define BJSON_DATATYPE_MAP32    38U /* 38, size[uint32], key0, value0, key1, value1, ...*/
#define BJSON_DATATYPE_MAP64    39U /* 39, size[uint64], key0, value0, key1, value1, ...*/

/*
 * strict primitives:
 * Strict primitives should be:
 * - used, when implementation (language) supports it,
 * - always implemented by the decoder (even if the decoding will loose the type),
 * - implemented by the encoder if possible,
 */

#define BJSON_DATATYPE_STRICT_FALSE        24U
#define BJSON_DATATYPE_STRICT_TRUE         25U
#define BJSON_DATATYPE_STRICT_INTEGER_ZERO 26U
#define BJSON_DATATYPE_STRICT_INTEGER_ONE  27U

#endif /* _BJSON_CONSTANTS_H_ */
