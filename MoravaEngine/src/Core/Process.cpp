#include "Process.h"

#include <cstdlib>
#include <system_error>

#if defined(_WIN32)
	#include <windows.h>
	#include <shellapi.h>
#else
	#include <cerrno>
	#include <cstring>
	#include <fcntl.h>
	#include <signal.h>
	#include <sys/wait.h>
	#include <unistd.h>
	#if defined(__APPLE__)
		#include <mach-o/dyld.h>
	#endif
#endif


namespace fs = std::filesystem;

namespace Process
{
	// The lines of a program's output as they come: a line is complete at its \n (a \r before it is dropped)
	class LineSplitter
	{
	public:
		explicit LineSplitter(const std::function<void(const std::string&)>& onLine) : m_OnLine(onLine) {}

		void Add(const char* data, size_t size)
		{
			for (size_t i = 0; i < size; i++)
			{
				if (data[i] == '\n')
				{
					Emit();
				}
				else
				{
					m_Line += data[i];
				}
			}
		}

		void Finish()
		{
			if (!m_Line.empty())
			{
				Emit();
			}
		}

	private:
		void Emit();

		std::function<void(const std::string&)> m_OnLine;
		std::string m_Line;
	};

#if defined(_WIN32)

	static std::wstring Widen(const std::string& text)
	{
		if (text.empty())
		{
			return std::wstring();
		}
		std::wstring out(MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), nullptr, 0), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), out.data(), (int)out.size());
		return out;
	}

	static std::string Narrow(const std::wstring& text, UINT codePage = CP_UTF8)
	{
		if (text.empty())
		{
			return std::string();
		}
		std::string out(WideCharToMultiByte(codePage, 0, text.data(), (int)text.size(), nullptr, 0, nullptr, nullptr), '\0');
		WideCharToMultiByte(codePage, 0, text.data(), (int)text.size(), out.data(), (int)out.size(), nullptr, nullptr);
		return out;
	}

	void LineSplitter::Emit()
	{
		if (!m_Line.empty() && m_Line.back() == '\r')
		{
			m_Line.pop_back();
		}
		// A console program writes UTF-8 or the console's code page (the .NET SDK: the OEM one, into a pipe)
		if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, m_Line.data(), (int)m_Line.size(), nullptr, 0) == 0 && !m_Line.empty())
		{
			std::wstring wide(MultiByteToWideChar(CP_OEMCP, 0, m_Line.data(), (int)m_Line.size(), nullptr, 0), L'\0');
			MultiByteToWideChar(CP_OEMCP, 0, m_Line.data(), (int)m_Line.size(), wide.data(), (int)wide.size());
			m_Line = Narrow(wide);
		}
		m_OnLine(m_Line);
		m_Line.clear();
	}

	// An argument as CommandLineToArgvW reads it back: quoted when needed, backslashes before a quote doubled
	static void AppendArgument(std::wstring& commandLine, const std::wstring& argument)
	{
		if (!commandLine.empty())
		{
			commandLine += L' ';
		}
		if (!argument.empty() && argument.find_first_of(L" \t\n\v\"") == std::wstring::npos)
		{
			commandLine += argument;
			return;
		}
		commandLine += L'"';
		for (size_t i = 0; ; i++)
		{
			size_t backslashes = 0;
			while (i < argument.size() && argument[i] == L'\\')
			{
				i++;
				backslashes++;
			}
			if (i == argument.size())
			{
				commandLine.append(backslashes * 2, L'\\');
				break;
			}
			if (argument[i] == L'"')
			{
				commandLine.append(backslashes * 2 + 1, L'\\');
			}
			else
			{
				commandLine.append(backslashes, L'\\');
			}
			commandLine += argument[i];
		}
		commandLine += L'"';
	}

	// The command line of a program; a .cmd or .bat runs in cmd.exe (/s: it drops only the outer quotes)
	static std::wstring CommandLine(const std::string& program, const std::vector<std::string>& arguments)
	{
		std::wstring line;
		AppendArgument(line, Widen(program));
		for (const std::string& argument : arguments)
		{
			AppendArgument(line, Widen(argument));
		}
		const std::string extension = fs::u8path(program).extension().string();
		if (_stricmp(extension.c_str(), ".cmd") == 0 || _stricmp(extension.c_str(), ".bat") == 0)
		{
			line = L"cmd.exe /d /s /c \"" + line + L"\"";
		}
		return line;
	}

	static std::string LastError()
	{
		const DWORD code = GetLastError();
		wchar_t* text = nullptr;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0,
			reinterpret_cast<wchar_t*>(&text), 0, nullptr);
		std::string message = text ? Narrow(text) : "error " + std::to_string(code);
		LocalFree(text);
		while (!message.empty() && (message.back() == '\n' || message.back() == '\r' || message.back() == '.'))
		{
			message.pop_back();
		}
		return message;
	}

	int Run(const std::string& program, const std::vector<std::string>& arguments, const std::string& workingDirectory,
		const std::function<void(const std::string&)>& onLine, std::string& error)
	{
		SECURITY_ATTRIBUTES inherit { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
		HANDLE readEnd = nullptr, writeEnd = nullptr;
		if (!CreatePipe(&readEnd, &writeEnd, &inherit, 0))
		{
			error = LastError();
			return -1;
		}
		SetHandleInformation(readEnd, HANDLE_FLAG_INHERIT, 0); // only the write end goes to the program

		STARTUPINFOW startup {};
		startup.cb = sizeof(startup);
		startup.dwFlags = STARTF_USESTDHANDLES;
		startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		startup.hStdOutput = writeEnd;
		startup.hStdError = writeEnd;
		PROCESS_INFORMATION info {};
		std::wstring commandLine = CommandLine(program, arguments);
		const std::wstring directory = Widen(workingDirectory);
		const BOOL started = CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
			directory.empty() ? nullptr : directory.c_str(), &startup, &info);
		CloseHandle(writeEnd); // the program has its own: the pipe ends when it does
		if (!started)
		{
			error = "'" + program + "' can't be started (" + LastError() + ")";
			CloseHandle(readEnd);
			return -1;
		}

		LineSplitter lines(onLine);
		char buffer[4096];
		DWORD read = 0;
		while (ReadFile(readEnd, buffer, sizeof(buffer), &read, nullptr) && read > 0)
		{
			lines.Add(buffer, read);
		}
		lines.Finish();
		CloseHandle(readEnd);

		WaitForSingleObject(info.hProcess, INFINITE);
		DWORD exitCode = 0;
		GetExitCodeProcess(info.hProcess, &exitCode);
		CloseHandle(info.hThread);
		CloseHandle(info.hProcess);
		return (int)exitCode;
	}

	bool Start(const std::string& program, const std::vector<std::string>& arguments, std::string& error)
	{
		STARTUPINFOW startup {};
		startup.cb = sizeof(startup);
		PROCESS_INFORMATION info {};
		std::wstring commandLine = CommandLine(program, arguments);
		const std::string extension = fs::u8path(program).extension().string();
		const bool script = _stricmp(extension.c_str(), ".cmd") == 0 || _stricmp(extension.c_str(), ".bat") == 0;
		if (!CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE, script ? CREATE_NO_WINDOW : 0, nullptr, nullptr,
			&startup, &info))
		{
			error = "'" + program + "' can't be started (" + LastError() + ")";
			return false;
		}
		CloseHandle(info.hThread);
		CloseHandle(info.hProcess);
		return true;
	}

	bool Open(const std::string& path, std::string& error)
	{
		const std::wstring file = Widen(path);
		const HINSTANCE result = ShellExecuteW(nullptr, L"open", file.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		if ((INT_PTR)result <= 32)
		{
			error = "'" + path + "' can't be opened (the system has no program for it?)";
			return false;
		}
		return true;
	}

	std::string FindOnPath(const std::string& name)
	{
		const DWORD size = GetEnvironmentVariableW(L"PATH", nullptr, 0);
		std::wstring path(size, L'\0');
		path.resize(GetEnvironmentVariableW(L"PATH", path.data(), size));
		const std::wstring wideName = Widen(name);
		size_t start = 0;
		while (start <= path.size())
		{
			size_t end = path.find(L';', start);
			if (end == std::wstring::npos)
			{
				end = path.size();
			}
			std::wstring folder = path.substr(start, end - start);
			if (folder.size() >= 2 && folder.front() == L'"' && folder.back() == L'"')
			{
				folder = folder.substr(1, folder.size() - 2);
			}
			if (!folder.empty())
			{
				for (const wchar_t* extension : { L".exe", L".cmd", L".bat", L"" })
				{
					std::error_code fileError;
					const fs::path candidate = fs::path(folder) / (wideName + extension);
					if (fs::is_regular_file(candidate, fileError))
					{
						return candidate.u8string();
					}
				}
			}
			start = end + 1;
		}
		return std::string();
	}

	fs::path GetExecutablePath()
	{
		std::wstring buffer(MAX_PATH, L'\0');
		DWORD length = 0;
		while ((length = GetModuleFileNameW(nullptr, buffer.data(), DWORD(buffer.size()))) == buffer.size())
		{
			buffer.resize(buffer.size() * 2);
		}
		buffer.resize(length);
		return buffer;
	}

#else

	void LineSplitter::Emit()
	{
		if (!m_Line.empty() && m_Line.back() == '\r')
		{
			m_Line.pop_back();
		}
		m_OnLine(m_Line);
		m_Line.clear();
	}

	static std::vector<char*> ArgumentVector(const std::string& program, const std::vector<std::string>& arguments)
	{
		std::vector<char*> argv;
		argv.push_back(const_cast<char*>(program.c_str()));
		for (const std::string& argument : arguments)
		{
			argv.push_back(const_cast<char*>(argument.c_str()));
		}
		argv.push_back(nullptr);
		return argv;
	}

	int Run(const std::string& program, const std::vector<std::string>& arguments, const std::string& workingDirectory,
		const std::function<void(const std::string&)>& onLine, std::string& error)
	{
		int pipeEnds[2];
		if (pipe(pipeEnds) != 0)
		{
			error = std::strerror(errno);
			return -1;
		}
		// The child tells through this pipe when exec failed (closed on exec: nothing comes when it worked)
		int execEnds[2];
		if (pipe(execEnds) != 0)
		{
			error = std::strerror(errno);
			close(pipeEnds[0]);
			close(pipeEnds[1]);
			return -1;
		}
		fcntl(execEnds[1], F_SETFD, FD_CLOEXEC);

		std::vector<char*> argv = ArgumentVector(program, arguments);
		const pid_t child = fork();
		if (child == 0)
		{
			// Only async-signal-safe calls until exec
			if (!workingDirectory.empty() && chdir(workingDirectory.c_str()) != 0)
			{
				const int code = errno;
				(void)!write(execEnds[1], &code, sizeof(code));
				_exit(127);
			}
			dup2(pipeEnds[1], STDOUT_FILENO);
			dup2(pipeEnds[1], STDERR_FILENO);
			close(pipeEnds[0]);
			close(pipeEnds[1]);
			close(execEnds[0]);
			execvp(program.c_str(), argv.data());
			const int code = errno;
			(void)!write(execEnds[1], &code, sizeof(code));
			_exit(127);
		}
		close(pipeEnds[1]);
		close(execEnds[1]);
		if (child < 0)
		{
			error = std::strerror(errno);
			close(pipeEnds[0]);
			close(execEnds[0]);
			return -1;
		}
		int execError = 0;
		const bool execFailed = read(execEnds[0], &execError, sizeof(execError)) == (ssize_t)sizeof(execError);
		close(execEnds[0]);

		LineSplitter lines(onLine);
		char buffer[4096];
		ssize_t count = 0;
		while ((count = read(pipeEnds[0], buffer, sizeof(buffer))) > 0 || (count < 0 && errno == EINTR))
		{
			if (count > 0)
			{
				lines.Add(buffer, (size_t)count);
			}
		}
		lines.Finish();
		close(pipeEnds[0]);

		int status = 0;
		while (waitpid(child, &status, 0) < 0 && errno == EINTR)
		{
		}
		if (execFailed)
		{
			error = "'" + program + "' can't be started (" + std::strerror(execError) + ")";
			return -1;
		}
		return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
	}

	bool Start(const std::string& program, const std::vector<std::string>& arguments, std::string& error)
	{
		// Twice forked: the program isn't the engine's child, so nothing has to wait for it
		std::vector<char*> argv = ArgumentVector(program, arguments);
		int execEnds[2];
		if (pipe(execEnds) != 0)
		{
			error = std::strerror(errno);
			return false;
		}
		fcntl(execEnds[1], F_SETFD, FD_CLOEXEC);
		const pid_t child = fork();
		if (child == 0)
		{
			setsid();
			if (fork() == 0)
			{
				close(execEnds[0]);
				execvp(program.c_str(), argv.data());
				const int code = errno;
				(void)!write(execEnds[1], &code, sizeof(code));
				_exit(127);
			}
			_exit(0);
		}
		close(execEnds[1]);
		if (child < 0)
		{
			error = std::strerror(errno);
			close(execEnds[0]);
			return false;
		}
		int status = 0;
		while (waitpid(child, &status, 0) < 0 && errno == EINTR)
		{
		}
		int execError = 0;
		const bool execFailed = read(execEnds[0], &execError, sizeof(execError)) == (ssize_t)sizeof(execError);
		close(execEnds[0]);
		if (execFailed)
		{
			error = "'" + program + "' can't be started (" + std::strerror(execError) + ")";
			return false;
		}
		return true;
	}

	bool Open(const std::string& path, std::string& error)
	{
#if defined(__APPLE__)
		return Start("open", { path }, error);
#else
		return Start("xdg-open", { path }, error);
#endif
	}

	std::string FindOnPath(const std::string& name)
	{
		const char* path = std::getenv("PATH");
		if (!path)
		{
			return std::string();
		}
		const std::string folders = path;
		size_t start = 0;
		while (start <= folders.size())
		{
			size_t end = folders.find(':', start);
			if (end == std::string::npos)
			{
				end = folders.size();
			}
			const std::string folder = folders.substr(start, end - start);
			if (!folder.empty())
			{
				const std::string candidate = folder + "/" + name;
				if (access(candidate.c_str(), X_OK) == 0)
				{
					return candidate;
				}
			}
			start = end + 1;
		}
		return std::string();
	}

	fs::path GetExecutablePath()
	{
		std::error_code error;
#if defined(__APPLE__)
		uint32_t size = 0;
		_NSGetExecutablePath(nullptr, &size);
		std::string buffer(size, '\0');
		_NSGetExecutablePath(buffer.data(), &size);
		return fs::canonical(buffer.c_str(), error);
#else
		return fs::read_symlink("/proc/self/exe", error);
#endif
	}

#endif
}
