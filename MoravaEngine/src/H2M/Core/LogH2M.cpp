#include "Hazel/Core/HazelLog.h"

#include "spdlog/sinks/stdout_color_sinks.h"

namespace Hazel {

	std::shared_ptr<spdlog::logger> HazelLog::s_CoreLogger;
	std::shared_ptr<spdlog::logger> HazelLog::s_ClientLogger;
	std::shared_ptr<spdlog::logger> HazelLog::s_EditorConsoleLogger;

	void HazelLog::Init()
	{
		spdlog::set_pattern("%^[%T] %n: %v%$");
		s_CoreLogger = spdlog::stdout_color_mt("HAZEL");
		s_CoreLogger->set_level(spdlog::level::trace);

		s_ClientLogger = spdlog::stdout_color_mt("APP");
		s_ClientLogger->set_level(spdlog::level::trace);
	}

}
