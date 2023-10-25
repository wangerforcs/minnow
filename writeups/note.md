cmake --build build --target checkn
可能会出现ld undefined reference to main，因为cmake编译时超时，多次编译可能导致出错，也可能不会。我自己就遇到了编译bytes_basic_sanitize未产生，