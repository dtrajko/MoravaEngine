#include "GameArchive.h"

#include <zlib.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <system_error>
#include <vector>


namespace fs = std::filesystem;

namespace GameArchive
{
	// The file, compressed as it is written
	class GzipFile
	{
	public:
		bool Open(const fs::path& path, std::string& error)
		{
			m_File.open(path, std::ios::binary | std::ios::trunc);
			if (!m_File)
			{
				error = "'" + path.generic_u8string() + "' can't be written";
				return false;
			}
			std::memset(&m_Stream, 0, sizeof(m_Stream));
			// 15 + 16: a gzip header and trailer around the deflate data; level 6, zlib's default
			if (deflateInit2(&m_Stream, 6, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
			{
				error = "zlib could not start";
				return false;
			}
			m_Started = true;
			return true;
		}

		~GzipFile()
		{
			if (m_Started)
			{
				deflateEnd(&m_Stream);
			}
		}

		bool Write(const void* data, size_t bytes)
		{
			return Deflate((const uint8_t*)data, bytes, Z_NO_FLUSH);
		}

		bool Finish()
		{
			return Deflate(nullptr, 0, Z_FINISH) && m_File.flush();
		}

	private:
		bool Deflate(const uint8_t* data, size_t bytes, int flush)
		{
			m_Stream.next_in = const_cast<Bytef*>(data);
			m_Stream.avail_in = (uInt)bytes;
			std::array<uint8_t, 1 << 16> out;
			int result = Z_OK;
			do
			{
				m_Stream.next_out = out.data();
				m_Stream.avail_out = (uInt)out.size();
				result = deflate(&m_Stream, flush);
				if (result == Z_STREAM_ERROR)
				{
					return false;
				}
				m_File.write((const char*)out.data(), (std::streamsize)(out.size() - m_Stream.avail_out));
			} while (m_Stream.avail_out == 0 || (flush == Z_FINISH && result != Z_STREAM_END));
			return (bool)m_File;
		}

		std::ofstream m_File;
		z_stream m_Stream;
		bool m_Started = false;
	};

	// A number in a header field: octal digits and a terminating zero
	static void WriteOctal(char* field, size_t size, uint64_t value)
	{
		std::snprintf(field, size, "%0*llo", (int)(size - 1), (unsigned long long)value);
	}

	// A ustar header: a name up to 100 bytes, or up to 255 split at a slash into a prefix (155) and the rest (100)
	static bool MakeHeader(const std::string& name, uint64_t size, uint32_t mode, char type, uint64_t time, std::array<char, 512>& header)
	{
		header.fill(0);
		std::string prefix, rest = name;
		if (name.size() > 100)
		{
			size_t slash = name.find('/', name.size() > 101 ? name.size() - 101 : 0);
			if (slash == std::string::npos || slash > 155)
			{
				return false;
			}
			prefix = name.substr(0, slash);
			rest = name.substr(slash + 1);
			if (rest.empty() || rest.size() > 100)
			{
				return false;
			}
		}
		std::memcpy(header.data(), rest.data(), rest.size());
		WriteOctal(header.data() + 100, 8, mode);
		WriteOctal(header.data() + 108, 8, 0); // owner and group: root, as an archive to unpack anywhere
		WriteOctal(header.data() + 116, 8, 0);
		WriteOctal(header.data() + 124, 12, size);
		WriteOctal(header.data() + 136, 12, time);
		header[156] = type;
		std::memcpy(header.data() + 257, "ustar", 6);
		std::memcpy(header.data() + 263, "00", 2);
		std::memcpy(header.data() + 345, prefix.data(), prefix.size());

		// The checksum: the bytes' sum with its own field counted as spaces
		std::memset(header.data() + 148, ' ', 8);
		uint32_t sum = 0;
		for (char byte : header)
		{
			sum += (uint8_t)byte;
		}
		std::snprintf(header.data() + 148, 8, "%06o", sum);
		header[155] = ' ';
		return true;
	}

	static bool IsElf(const fs::path& file)
	{
		std::ifstream stream(file, std::ios::binary);
		char magic[4] = {};
		stream.read(magic, sizeof(magic));
		return stream && std::memcmp(magic, "\x7F" "ELF", 4) == 0;
	}

	bool WriteTarGz(const fs::path& folder, const std::string& topName, const fs::path& archive, std::string& error)
	{
		// Every entry, in name order, so the same folder makes the same archive
		std::vector<fs::path> entries;
		std::error_code fileError;
		for (fs::recursive_directory_iterator it(folder, fileError), end; !fileError && it != end; it.increment(fileError))
		{
			entries.push_back(it->path());
		}
		if (fileError)
		{
			error = "'" + folder.generic_u8string() + "' can't be read: " + fileError.message();
			return false;
		}
		std::sort(entries.begin(), entries.end());

		GzipFile out;
		if (!out.Open(archive, error))
		{
			return false;
		}
		const uint64_t time = (uint64_t)std::time(nullptr);
		std::array<char, 512> header;
		auto writeHeader = [&](const std::string& name, uint64_t size, uint32_t mode, char type)
		{
			if (!MakeHeader(name, size, mode, type, time, header))
			{
				error = "the path '" + name + "' is too long for a tar file";
				return false;
			}
			return out.Write(header.data(), header.size());
		};

		if (!writeHeader(topName + "/", 0, 0755, '5'))
		{
			return false;
		}
		std::vector<char> buffer(1 << 20);
		for (const fs::path& entry : entries)
		{
			const std::string name = topName + "/" + entry.lexically_relative(folder).generic_u8string();
			if (fs::is_directory(entry, fileError))
			{
				if (!writeHeader(name + "/", 0, 0755, '5'))
				{
					return false;
				}
				continue;
			}
			const uint64_t size = fs::file_size(entry, fileError);
			std::ifstream in(entry, std::ios::binary);
			if (fileError || !in)
			{
				error = "'" + entry.generic_u8string() + "' can't be read";
				return false;
			}
			if (!writeHeader(name, size, IsElf(entry) ? 0755 : 0644, '0'))
			{
				return false;
			}
			uint64_t left = size;
			while (left > 0)
			{
				const size_t chunk = (size_t)std::min<uint64_t>(left, buffer.size());
				in.read(buffer.data(), (std::streamsize)chunk);
				if (!in || !out.Write(buffer.data(), chunk))
				{
					error = "'" + entry.generic_u8string() + "' could not be packed";
					return false;
				}
				left -= chunk;
			}
			const size_t padding = (size_t)((512 - size % 512) % 512); // each file fills whole blocks
			static const char zeros[512] = {};
			if (padding && !out.Write(zeros, padding))
			{
				error = "the archive could not be written";
				return false;
			}
		}

		// The end: two empty blocks
		static const char end[1024] = {};
		if (!out.Write(end, sizeof(end)) || !out.Finish())
		{
			error = "'" + archive.generic_u8string() + "' could not be written (is the disk full?)";
			return false;
		}
		return true;
	}
}
