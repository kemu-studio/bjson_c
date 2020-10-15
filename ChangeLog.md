# 2.0.0 (2020-10-14)
- Decoder callbacks return bjson_decodeCallbackResult_t codes:
  - bjson_decodeCallbackResult_Continue,
  - bjson_decodeCallbackResult_Abort,
  - bjson_decodeCallbackResult_StepOver (Reserved for future use),
  - bjson_decodeCallbackResult_StepOutside (Reserved for future use).
- C++ wrappers: added encodeKeyAndValueXxx() methods.

# 1.1.0 (2020-09-03)
- Added C++ wrappers.

# 1.0.0 (2017-06-07)
- Initial release.
