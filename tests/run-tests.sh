#
# Copyright (c) 2007-2014, Lloyd Hilaiel <me@lloyd.io>
#               2017, Kemu Studio <visit ke.mu>.
#
# This file was originally a part of the yajl project
# http://lloyd.github.io/yajl/.
#
# Adapted to bjson tests by Sylwester Wysocki <sw@ke.mu>
# and Roman Pietrzak <rp@ke.mu>.
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

#!/bin/sh

ECHO=`which echo`

DIFF_FLAGS="-u"
case "$(uname)" in
  *W32*)
    DIFF_FLAGS="-wu"
    ;;
esac

if [ -z "$testBin" ]; then
  testBin="$1"
fi

# find test binary on both platforms.  allow the caller to force a
# particular test binary (useful for non-cmake build systems).
if [ -z "$testBin" ]; then
    testBin="./bjson-test.exe"
    if [ ! -x $testBin ] ; then
        testBin="./bjson-test"
        if [ ! -x $testBin ] ; then
            testBin="../build/bin/bjson-test.exe"
            if [ ! -x $testBin ] ; then
              testBin="../build/bin/bjson-test"
                if [  ! -x $testBin ] ; then
                  ${ECHO} "cannot execute test binary: '$testBin'"
                  exit 1;
                fi
            fi
        fi
    fi
fi

${ECHO} "using test binary: $testBin"

testBinShort=`basename $testBin`

testsSucceeded=0
testsTotal=0

for mode in "encode" "decode"; do
  for file in cases/*.bjson ; do
    corruptedTest=0
    skipTest=0
    status="OK"

    # if the filename starts with dc_, we disallow comments for this test
    case $(basename $file) in
      corrupted-*)
        corruptedTest=1;
        ;;
    esac
    fileShort=`basename $file`
    testName=`echo $fileShort | sed -e 's/\.bjson$//'`

    ${ECHO} -n " $mode: test ($testName): "
    iter=1

    if [ "$mode" = "encode" ]; then

      #
      # Encode test.
      #

      if [ "$corruptedTest" -eq "1" ]; then
        skipTest=1
        status="SKIPPED"

      else
        $testBin "--encode" < $file > ${file}.test 2>&1
        diff ${DIFF_FLAGS} ${file} ${file}.test > ${file}.out
        if [ $? -eq 0 ] ; then
          testsSucceeded=$(( $testsSucceeded + 1 ))
        else
          status="FAIL"
          ${ECHO} $status
          exit 1
        fi
      fi

    else

      #
      # Decode test.
      #

      # parse with a read buffer size ranging from 1-31 to stress stream parsing
      while [ $iter -lt 32  ] && [ $status = "OK" ] ; do
        ${ECHO} -n "."
        $testBin $allowPartials $allowComments $allowGarbage $allowMultiple -b $iter < $file > ${file}.test  2>&1
        diff ${DIFF_FLAGS} ${file}.gold ${file}.test > ${file}.out
        if [ $? -eq 0 ] ; then
          if [ $iter -eq 31 ] ; then testsSucceeded=$(( $testsSucceeded + 1 )) ; fi
        else
          status="FAIL"
          iter=32
          ${ECHO} $status
          cat ${file}.out
          exit 1
        fi
        iter=$(( iter + 1 ))
        rm ${file}.test ${file}.out
      done
    fi

    # Report test result.
    ${ECHO} $status

    if [ "$skipTest" -eq "0" ]; then
      testsTotal=$(( testsTotal + 1 ))
    fi
  done

  ${ECHO} $testsSucceeded/$testsTotal tests successful

  if [ $testsSucceeded != $testsTotal ] ; then
    exit 1
  fi
done

exit 0
