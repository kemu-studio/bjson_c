# Customized arguments passed to underlying clang-tidy tool.
CLANG_ARG_INCLUDE="-Ibuild/include"
CLANG_ARG_FIX=""

# Add -f or --fix to automatically fix
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -f|--fix) CLANG_ARG_FIX="--fix -fix-errors"; shift ;;
        -test|--test) test="$2"; shift ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

echo "[ $0 ] Should fix ? $CLANG_ARG_FIX"

parse_one_file () {
  param_src=$1

  echo "[ $0 ] parsing '$param_src' ..."

  clang-tidy               \
    --header-filter=.*     \
    --warnings-as-errors=* \
    $CLANG_ARG_FIX         \
    $param_src             \
    --                     \
    $CLANG_ARG_INCLUDE
}

# Entry point.
parse_one_file "./src/src-c/*.c"
parse_one_file "./src/src-c/*.h"
parse_one_file "./src/src-cpp/*.hpp"