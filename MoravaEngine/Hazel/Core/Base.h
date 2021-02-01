#pragma once

#include "../../CommonValues.h"

#include <memory>


// __VA_ARGS__ expansion to get past MSVC "bug"
#define HZ_EXPAND_VARGS(x) x

#define BIT(x) (1 << x)

#define APP_BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)
#define HZ_BIND_EVENT_FN(fn) std::bind(&##fn, this, std::placeholders::_1)

template<typename T>
using Scope = std::unique_ptr<T>;
template<typename T, typename ... Args>
constexpr Scope<T> CreateScope(Args&& ... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
using Ref = std::shared_ptr<T>;
template<typename T, typename ... Args>
constexpr Ref<T> CreateRef(Args&& ... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}
