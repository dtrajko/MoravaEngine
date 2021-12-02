#pragma once

#include "H2M/Core/BaseH2M.h"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"


namespace H2M
{

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
#define H2M_CORE_TRACE(...)	LogH2M::GetCoreLogger()->trace(__VA_ARGS__)
#define H2M_CORE_INFO(...)	LogH2M::GetCoreLogger()->info(__VA_ARGS__)
#define H2M_CORE_WARN(...)	LogH2M::GetCoreLogger()->warn(__VA_ARGS__)
#define H2M_CORE_ERROR(...)	LogH2M::GetCoreLogger()->error(__VA_ARGS__)
#define H2M_CORE_FATAL(...)	LogH2M::GetCoreLogger()->critical(__VA_ARGS__)

// Client Logging Macros
#define H2M_TRACE(...)	LogH2M::GetClientLogger()->trace(__VA_ARGS__)
#define H2M_INFO(...)	LogH2M::GetClientLogger()->info(__VA_ARGS__)
#define H2M_WARN(...)	LogH2M::GetClientLogger()->warn(__VA_ARGS__)
#define H2M_ERROR(...)	LogH2M::GetClientLogger()->error(__VA_ARGS__)
#define H2M_FATAL(...)	LogH2M::GetClientLogger()->critical(__VA_ARGS__)

// Editor Console Logging Macros
#define H2M_CONSOLE_LOG_TRACE(...)	LogH2M::GetEditorConsoleLogger()->trace(__VA_ARGS__)
#define H2M_CONSOLE_LOG_INFO(...)	LogH2M::GetEditorConsoleLogger()->info(__VA_ARGS__)
#define H2M_CONSOLE_LOG_WARN(...)	LogH2M::GetEditorConsoleLogger()->warn(__VA_ARGS__)
#define H2M_CONSOLE_LOG_ERROR(...)	LogH2M::GetEditorConsoleLogger()->error(__VA_ARGS__)
#define H2M_CONSOLE_LOG_FATAL(...)	LogH2M::GetEditorConsoleLogger()->critical(__VA_ARGS__)
