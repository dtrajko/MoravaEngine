#pragma once

#include "Hazel/Core/Base.h"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace Hazel {

	class HazelLog
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};

}

// Core Logging Macros
#define HZ_CORE_TRACE(...)	Hazel::HazelLog::GetCoreLogger()->trace(__VA_ARGS__)
#define HZ_CORE_INFO(...)	Hazel::HazelLog::GetCoreLogger()->info(__VA_ARGS__)
#define HZ_CORE_WARN(...)	Hazel::HazelLog::GetCoreLogger()->warn(__VA_ARGS__)
#define HZ_CORE_ERROR(...)	Hazel::HazelLog::GetCoreLogger()->error(__VA_ARGS__)
#define HZ_CORE_FATAL(...)	Hazel::HazelLog::GetCoreLogger()->critical(__VA_ARGS__)

// Client Logging Macros
#define HZ_TRACE(...)	Hazel::HazelLog::GetClientLogger()->trace(__VA_ARGS__)
#define HZ_INFO(...)	Hazel::HazelLog::GetClientLogger()->info(__VA_ARGS__)
#define HZ_WARN(...)	Hazel::HazelLog::GetClientLogger()->warn(__VA_ARGS__)
#define HZ_ERROR(...)	Hazel::HazelLog::GetClientLogger()->error(__VA_ARGS__)
#define HZ_FATAL(...)	Hazel::HazelLog::GetClientLogger()->critical(__VA_ARGS__)
