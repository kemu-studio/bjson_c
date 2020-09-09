#
# Copyright (c) 2017,2020 by Kemu Studio (visit ke.mu)
#
# Author(s): Sylwester Wysocki <sw@ke.mu>,
#            Roman Pietrzak <rp@ke.mu>
#
# This file is a part of the KEMU Binary JSON library.
# See http://bjson.org for more.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

set(CMAKE_C_FLAGS "-O3")

set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../build/lib)

set(HEADER_FILES bjson-common.h bjson-decode.h bjson-encode.h
                 bjson-constants.h bjson-debug.h)

set(SOURCES bjson-common.c bjson-decode.c bjson-encode.c)

add_library (bjson_c ${SOURCES} ${HEADER_FILES})

install(FILES ${HEADER_FILES}
        DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../../build/include/bjson)
