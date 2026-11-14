#include "FrameLimiter.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#else
#include <filesystem>
#include <fstream>
#include <string>
#endif


namespace FrameLimiter
{
	namespace
	{
		using Clock = std::chrono::steady_clock;

		std::atomic<int> s_Limit{ 0 };
		std::atomic<int> s_BatteryLimit{ 0 };
		std::atomic<int> s_VSyncRate{ 0 };
		bool s_ForcedBattery = false;

		bool s_OnBattery = false;
		Clock::time_point s_PowerChecked;
		bool s_PowerKnown = false;

		Clock::time_point s_Due;     // when the next frame may start
		bool s_HasDue = false;
		const char* s_Reason = "";

		// The system's own answer: true only when it says the computer runs on its battery
		bool AskSystem()
		{
#if defined(_WIN32)
			SYSTEM_POWER_STATUS status{};
			return GetSystemPowerStatus(&status) && status.ACLineStatus == 0;
#elif defined(__APPLE__)
			bool battery = false;
			if (CFTypeRef info = IOPSCopyPowerSourcesInfo())
			{
				if (CFStringRef type = IOPSGetProvidingPowerSourceType(info))
				{
					battery = CFStringCompare(type, CFSTR(kIOPMBatteryPowerKey), 0) == kCFCompareEqualTo;
				}
				CFRelease(info);
			}
			return battery;
#else
			// Linux: a mains supply that is online means plugged in; else a battery that discharges
			namespace fs = std::filesystem;
			std::error_code error;
			const auto read = [](const fs::path& file)
			{
				std::ifstream in(file);
				std::string text;
				std::getline(in, text);
				return text;
			};
			bool discharging = false;
			for (const fs::directory_entry& entry : fs::directory_iterator("/sys/class/power_supply", error))
			{
				const std::string type = read(entry.path() / "type");
				if (type == "Mains" && read(entry.path() / "online") == "1")
				{
					return false;
				}
				if (type == "Battery" && read(entry.path() / "status") == "Discharging")
				{
					discharging = true;
				}
			}
			return discharging;
#endif
		}

		// Waits until the time point, without keeping a CPU core busy: the system's sleep until shortly before, then a few
		// short yields (a sleep can be late by a millisecond or more)
		void WaitUntil(Clock::time_point due)
		{
			constexpr auto Spin = std::chrono::microseconds(500);
#if defined(_WIN32)
			// A high-resolution timer (Windows 10 1803 and later); without it, Sleep at the system's timer resolution
			static HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
			const auto sleep = due - Clock::now() - Spin;
			if (sleep > std::chrono::microseconds(0))
			{
				if (timer)
				{
					LARGE_INTEGER dueTime;
					dueTime.QuadPart = -(LONGLONG)(std::chrono::duration_cast<std::chrono::nanoseconds>(sleep).count() / 100); // relative, 100 ns units
					if (SetWaitableTimer(timer, &dueTime, 0, nullptr, nullptr, FALSE))
					{
						WaitForSingleObject(timer, INFINITE);
					}
				}
				else
				{
					Sleep((DWORD)std::chrono::duration_cast<std::chrono::milliseconds>(sleep).count());
				}
			}
#else
			const auto sleep = due - Clock::now() - Spin;
			if (sleep > std::chrono::microseconds(0))
			{
				std::this_thread::sleep_for(sleep);
			}
#endif
			while (Clock::now() < due)
			{
				std::this_thread::yield();
			}
		}
	}

	void SetLimit(int framesPerSecond)
	{
		s_Limit.store(std::max(framesPerSecond, 0));
	}

	int GetLimit()
	{
		return s_Limit.load();
	}

	void SetBatteryLimit(int framesPerSecond)
	{
		s_BatteryLimit.store(std::max(framesPerSecond, 0));
	}

	int GetBatteryLimit()
	{
		return s_BatteryLimit.load();
	}

	bool IsOnBattery()
	{
		if (s_ForcedBattery)
		{
			return true;
		}
		const Clock::time_point now = Clock::now();
		if (!s_PowerKnown || now - s_PowerChecked >= std::chrono::seconds(2))
		{
			s_OnBattery = AskSystem();
			s_PowerChecked = now;
			s_PowerKnown = true;
		}
		return s_OnBattery;
	}

	void ForceOnBattery(bool onBattery)
	{
		s_ForcedBattery = onBattery;
	}

	void SetVSyncRate(int hertz)
	{
		s_VSyncRate.store(std::max(hertz, 0));
	}

	int GetActiveLimit()
	{
		int limit = s_Limit.load();
		s_Reason = "";
		const int battery = s_BatteryLimit.load();
		if (battery > 0 && (limit == 0 || battery < limit) && IsOnBattery())
		{
			limit = battery;
			s_Reason = "on battery";
		}
		// VSync already stops at the display's rate: a limit at or above it would only make the frames uneven
		const int vsync = s_VSyncRate.load();
		if (limit > 0 && vsync > 0 && limit >= vsync - 1)
		{
			s_Reason = "VSync holds it";
			return 0;
		}
		return limit;
	}

	const char* GetActiveReason()
	{
		return s_Reason;
	}

	void Wait()
	{
		const int limit = GetActiveLimit();
		const Clock::time_point now = Clock::now();
		if (limit <= 0)
		{
			s_HasDue = false;
			return;
		}
		const auto period = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / limit));
		if (s_HasDue && now < s_Due)
		{
			WaitUntil(s_Due);
		}
		// The next frame one period after this one was due; after a long frame (a load, a hitch) counted from now, so the
		// frames after it don't hurry to catch up
		s_Due = s_HasDue && now < s_Due + period ? s_Due + period : Clock::now() + period;
		s_HasDue = true;
	}
}
