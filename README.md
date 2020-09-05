# bjson_c: binary JSON for C (encoder/decoder)
- The reference BJSON implementation in C language,
- For C++ wrappers please check out https://github.com/kemu-studio/bjson_c/tree/master/src/src-cpp,
- For JavaScript (Node.js) implementation please visit https://github.com/kemu-studio/bjson_node,
- For protocol details please visit http://bjson.org/.

## Usage
- For usage examples please visit *examples* directory. The available examples are:

| File name               | Purpose | Target programming language |
| ------------------------|---------|-----------------------------|
| json-example-decode.c   | decode  | pure C                      |
| json-example-encode.c   | encode  | pure C                      |
| json-example-decode.cpp | decode  | C++                         |
| json-example-encode.cpp | encode  | C++                         |

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
