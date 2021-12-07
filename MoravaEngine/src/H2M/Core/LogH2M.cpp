/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "H2M/Core/LogH2M.h"

#include "spdlog/sinks/stdout_color_sinks.h"

namespace H2M
{

	std::shared_ptr<spdlog::logger> LogH2M::s_CoreLogger;
	std::shared_ptr<spdlog::logger> LogH2M::s_ClientLogger;

	void LogH2M::Init()
	{
		spdlog::set_pattern("%^[%T] %n: %v%$");
		s_CoreLogger = spdlog::stdout_color_mt("HAZEL");
		s_CoreLogger->set_level(spdlog::level::trace);

		s_ClientLogger = spdlog::stdout_color_mt("APP");
		s_ClientLogger->set_level(spdlog::level::trace);
	}

}
