# bjson_c: binary JSON for C (encoder/decoder)
- The reference BJSON implementation in C language,
- For C++ wrappers please check out [src/src-cpp directory](src/src-cpp) in the source tree,
- For JavaScript (Node.js) implementation please visit https://github.com/kemu-studio/bjson_node,
- For protocol details please visit http://bjson.org/.

## Build
```
  $ ./01_prepare.sh 
  $ ./02_build.sh
```

## Usage
- For usage examples please visit [examples directory](examples). The available examples are:

| File name                                                       | Purpose | Target programming language |
| ----------------------------------------------------------------|---------|-----------------------------|
| [bjson-example-decode.c](examples/bjson-example-decode.c)       | decode  | pure C                      |
| [bjson-example-encode.c](examples/bjson-example-encode.c)       | encode  | pure C                      |
| [bjson-example-decoder.cpp](examples/bjson-example-decoder.cpp) | decode  | C++                         |
| [bjson-example-encoder.cpp](examples/bjson-example-encoder.cpp) | encode  | C++                         |

## Code formatting (for contributors only)

### C coding style
1. File names:
```
  bjson-xxx.c
  bjson-xxx.h
```  
  
2. Struct and custom types:
```
  bjson_callbacks_t
  bjson_status_t
```

3. Static, this module only (private) functions:
```
  static void _doSomeInternallStuff()
```  

4. Comments are /* */ style always.

5. Enter before begin of block:
```
  void foo()
  {
    struct _bar_t
    {
      int x;
      int y;
      int z;
    };
  }
```

6. Global variables (singletons):
```  
  static int g_some_private_variable = 1;
```  
