/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "Core/CommonValues.h"

#include <memory>


// __VA_ARGS__ expansion to get past MSVC "bug"
#define H2M_EXPAND_VARGS(x) x

#define BIT(x) (1 << x)

#define APP_BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)
#define H2M_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

template<typename T>
using ScopeH2M = std::unique_ptr<T>;
template<typename T, typename ... Args>
constexpr ScopeH2M<T> CreateScopeH2M(Args&& ... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
using RefH2M = std::shared_ptr<T>;
template<typename T, typename ... Args>
constexpr RefH2M<T> CreateRefH2M(Args&& ... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

using byte = uint8_t;
