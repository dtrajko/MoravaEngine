/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

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

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};

}

// Core Logging Macros
#define H2M_CORE_TRACE(...)	H2M::LogH2M::GetCoreLogger()->trace(__VA_ARGS__)
#define H2M_CORE_INFO(...)	H2M::LogH2M::GetCoreLogger()->info(__VA_ARGS__)
#define H2M_CORE_WARN(...)	H2M::LogH2M::GetCoreLogger()->warn(__VA_ARGS__)
#define H2M_CORE_ERROR(...)	H2M::LogH2M::GetCoreLogger()->error(__VA_ARGS__)
#define H2M_CORE_FATAL(...)	H2M::LogH2M::GetCoreLogger()->critical(__VA_ARGS__)

// Client Logging Macros
#define H2M_TRACE(...)	H2M::LogH2M::GetClientLogger()->trace(__VA_ARGS__)
#define H2M_INFO(...)	H2M::LogH2M::GetClientLogger()->info(__VA_ARGS__)
#define H2M_WARN(...)	H2M::LogH2M::GetClientLogger()->warn(__VA_ARGS__)
#define H2M_ERROR(...)	H2M::LogH2M::GetClientLogger()->error(__VA_ARGS__)
#define H2M_FATAL(...)	H2M::LogH2M::GetClientLogger()->critical(__VA_ARGS__)
