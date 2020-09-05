# Adjust code formatting for all c, h, cpp and hpp files.
clang-format -verbose -i \
  ./src/src-c/*.c        \
  ./src/src-c/*.h        \
  ./src/src-cpp/*.cpp    \
  ./src/src-cpp/*.hpp