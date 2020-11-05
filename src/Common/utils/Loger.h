#pragma once
#include <iostream>

#ifndef LOG_S
#define LOG_S(...) \
    std::cout << "\n" << __FILE__ << "<" << __LINE__ << ">:\n"; printf(__VA_ARGS__); std::cout << "\n"
#endif

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif
