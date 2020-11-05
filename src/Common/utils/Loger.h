#pragma once
#include <iostream>

#ifndef LOG_S
#define LOG_S(...) \
    std::cout << "\n" << __FILE__ << "<" << __LINE__ << ">:\n"; printf(__VA_ARGS__); std::cout << "\n"
#endif

#ifndef ThrowException
#define ThrowException(x) \
	{ \
		std::string ss_throw_msg = std::string(__FILE__) + std::to_string(__LINE__ ) +": \t" + std::string(#x);  \
		throw std::exception(ss_throw_msg.c_str()); \
	}
#endif

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif
