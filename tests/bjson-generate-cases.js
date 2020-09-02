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

require('coffee-script/register')
let K = require('kcore')
let glob = require('glob')
let fs = require('fs')
require('kbjson')

glob('./cases/*.json', function (err, arrayOfFiles)
{
  for (let idx in arrayOfFiles)
  {
    let srcPath = arrayOfFiles[idx]
    let dstPath = srcPath.replace('.json', '.bjson')
    let src     = fs.readFileSync(srcPath).toString()

    try
    {
      let srcJson = JSON.parse(src)
      let dst     = K.BJSON.encode(srcJson)

      fs.writeFileSync(dstPath, dst)
    }
    catch (err)
    {
      console.log('ERROR: Cannot to encode', srcPath)
    }
  }
})
