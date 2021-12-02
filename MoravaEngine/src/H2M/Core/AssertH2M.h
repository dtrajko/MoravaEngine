#pragma once

#include "H2M/Core/LogH2M.h"

#include <signal.h>


#ifdef HZ_PLATFORM_WINDOWS
#define H2M_DEBUGBREAK __debugbreak()
#elif HZ_PLATFORM_LINUX
#define HZ_DEBUGBREAK raise(SIGTRAP)
#endif

#ifdef HZ_DEBUG
	#define H2M_ENABLE_ASSERTS
#endif

#ifdef H2M_ENABLE_ASSERTS
	#define H2M_ASSERT_NO_MESSAGE(condition) { if(!(condition)) { H2M_ERROR("Assertion Failed"); H2M_DEBUGBREAK; } } // __debugbreak() not supported by GCC
	#define H2M_ASSERT_MESSAGE(condition, ...) { if(!(condition)) { H2M_ERROR("Assertion Failed: {0}", __VA_ARGS__); H2M_DEBUGBREAK; } } // __debugbreak() not supported by GCC

	#define H2M_ASSERT_RESOLVE(arg1, arg2, macro, ...) macro
	#define H2M_GET_ASSERT_MACRO(...) H2M_EXPAND_VARGS(H2M_ASSERT_RESOLVE(__VA_ARGS__, H2M_ASSERT_MESSAGE, H2M_ASSERT_NO_MESSAGE))

	#define H2M_ASSERT(...) H2M_EXPAND_VARGS( H2M_GET_ASSERT_MACRO(__VA_ARGS__)(__VA_ARGS__) )
	#define H2M_CORE_ASSERT(...) H2M_EXPAND_VARGS( H2M_GET_ASSERT_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#else
	#define H2M_ASSERT(...)
	#define H2M_CORE_ASSERT(...)
#endif

#ifdef H2M_ENABLE_VERIFY
#define H2M_VERIFY_NO_MESSAGE(condition) { if(!(condition)) { H2M_ERROR("Verify Failed"); __debugbreak(); } }
#define H2M_VERIFY_MESSAGE(condition, ...) { if(!(condition)) { H2M_ERROR("Verify Failed: {0}", __VA_ARGS__); __debugbreak(); } }

#define H2M_VERIFY_RESOLVE(arg1, arg2, macro, ...) macro
#define H2M_GET_VERIFY_MACRO(...) H2M_EXPAND_VARGS(H2M_VERIFY_RESOLVE(__VA_ARGS__, H2M_VERIFY_MESSAGE, H2M_VERIFY_NO_MESSAGE))

#define H2M_VERIFY(...) H2M_EXPAND_VARGS( H2M_GET_VERIFY_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#define H2M_CORE_VERIFY(...) H2M_EXPAND_VARGS( H2M_GET_VERIFY_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#else
#define H2M_VERIFY(...)
#define H2M_CORE_VERIFY(...)
#endif
