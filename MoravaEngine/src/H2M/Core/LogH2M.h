#pragma once

#include "H2M/Core/BaseH2M.h"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace H2M {

	class LogH2M
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetEditorConsoleLogger() { return s_EditorConsoleLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
		static std::shared_ptr<spdlog::logger> s_EditorConsoleLogger;
	};

}

// Core Logging Macros
#define HZ_CORE_TRACE(...)	H2M::LogH2M::GetCoreLogger()->trace(__VA_ARGS__)
#define HZ_CORE_INFO(...)	H2M::LogH2M::GetCoreLogger()->info(__VA_ARGS__)
#define HZ_CORE_WARN(...)	H2M::LogH2M::GetCoreLogger()->warn(__VA_ARGS__)
#define HZ_CORE_ERROR(...)	H2M::LogH2M::GetCoreLogger()->error(__VA_ARGS__)
#define HZ_CORE_FATAL(...)	H2M::LogH2M::GetCoreLogger()->critical(__VA_ARGS__)

// Client Logging Macros
#define HZ_TRACE(...)	H2M::LogH2M::GetClientLogger()->trace(__VA_ARGS__)
#define HZ_INFO(...)	H2M::LogH2M::GetClientLogger()->info(__VA_ARGS__)
#define HZ_WARN(...)	H2M::LogH2M::GetClientLogger()->warn(__VA_ARGS__)
#define HZ_ERROR(...)	H2M::LogH2M::GetClientLogger()->error(__VA_ARGS__)
#define HZ_FATAL(...)	H2M::LogH2M::GetClientLogger()->critical(__VA_ARGS__)

// Editor Console Logging Macros
#define HZ_CONSOLE_LOG_TRACE(...)	H2M::LogH2M::GetEditorConsoleLogger()->trace(__VA_ARGS__)
#define HZ_CONSOLE_LOG_INFO(...)	H2M::LogH2M::GetEditorConsoleLogger()->info(__VA_ARGS__)
#define HZ_CONSOLE_LOG_WARN(...)	H2M::LogH2M::GetEditorConsoleLogger()->warn(__VA_ARGS__)
#define HZ_CONSOLE_LOG_ERROR(...)	H2M::LogH2M::GetEditorConsoleLogger()->error(__VA_ARGS__)
#define HZ_CONSOLE_LOG_FATAL(...)	H2M::LogH2M::GetEditorConsoleLogger()->critical(__VA_ARGS__)
