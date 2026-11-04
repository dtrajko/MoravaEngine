#include "TextureCook.h"

#include "CookFiles.h"

#include "Core/Log.h"

#include "bc7decomp.h"
#include "bc7enc.h"
#include "rgbcx.h"
#include "stb_image.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <set>
#include <system_error>
#include <thread>
#include <type_traits>

#if defined(_WIN32)
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
#endif


namespace fs = std::filesystem;

namespace TextureCook
{
	// Raise it when the file's layout, the mips or the encoder's settings change: older files are cooked again
	static constexpr uint32_t CookerVersion = 1;
	static constexpr uint32_t BlockSize = 16; // bytes, in BC7 and in BC5

	// ---- Mips

	// An image as linear values per channel: a color map's RGB is sRGB in the source, and averaging has to happen in linear
	// light (what the GPU's blit does for an sRGB image), or mips of fine detail come out too dark
	struct Pixels
	{
		uint32_t Width = 0;
		uint32_t Height = 0;
		std::vector<float> Values; // 4 per pixel
	};

	static float SrgbToLinear(float value)
	{
		return value <= 0.04045f ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, 2.4f);
	}

	static const float* GetSrgbToLinearTable()
	{
		static const std::vector<float> s_Table = []()
		{
			std::vector<float> table(256);
			for (int i = 0; i < 256; i++)
			{
				table[i] = SrgbToLinear(i / 255.0f);
			}
			return table;
		}();
		return s_Table.data();
	}

	// With 16384 steps the darkest values, where sRGB is steepest, are within a fifth of an 8-bit step
	static const uint8_t* GetLinearToSrgbTable()
	{
		static const std::vector<uint8_t> s_Table = []()
		{
			std::vector<uint8_t> table(16384);
			for (int i = 0; i < 16384; i++)
			{
				const float value = i / 16383.0f;
				const float encoded = value <= 0.0031308f ? value * 12.92f : 1.055f * std::pow(value, 1.0f / 2.4f) - 0.055f;
				table[i] = (uint8_t)std::min(255.0f, encoded * 255.0f + 0.5f);
			}
			return table;
		}();
		return s_Table.data();
	}

	static Pixels ToPixels(const uint8_t* rgba, uint32_t width, uint32_t height, MapKind kind)
	{
		const float* srgb = GetSrgbToLinearTable();
		Pixels pixels;
		pixels.Width = width;
		pixels.Height = height;
		pixels.Values.resize((size_t)width * height * 4);
		const size_t count = pixels.Values.size();
		for (size_t i = 0; i < count; i++)
		{
			const bool encoded = kind == MapKind::Color && (i & 3) != 3;
			pixels.Values[i] = encoded ? srgb[rgba[i]] : rgba[i] / 255.0f;
		}
		return pixels;
	}

	static void ToBytes(const Pixels& pixels, MapKind kind, std::vector<uint8_t>& rgba)
	{
		const uint8_t* srgb = GetLinearToSrgbTable();
		const size_t count = pixels.Values.size();
		rgba.resize(count);
		for (size_t i = 0; i < count; i++)
		{
			const float value = std::min(std::max(pixels.Values[i], 0.0f), 1.0f);
			const bool encoded = kind == MapKind::Color && (i & 3) != 3;
			rgba[i] = encoded ? srgb[(size_t)(value * 16383.0f + 0.5f)] : (uint8_t)(value * 255.0f + 0.5f);
		}
	}

	// The next level, half the size, each pixel the average of 2 x 2 pixels (of 2 along a side already 1 long; an odd side's
	// last row or column is left out, as in the GPU's blit). A normal map's averages are made unit length again
	static Pixels MakeNextLevel(const Pixels& source, MapKind kind)
	{
		Pixels level;
		level.Width = std::max(source.Width / 2, 1u);
		level.Height = std::max(source.Height / 2, 1u);
		level.Values.resize((size_t)level.Width * level.Height * 4);
		const uint32_t stepX = source.Width > 1 ? 1 : 0;
		const uint32_t stepY = source.Height > 1 ? 1 : 0;
		const float weight = 1.0f / ((stepX + 1) * (stepY + 1));
		for (uint32_t y = 0; y < level.Height; y++)
		{
			for (uint32_t x = 0; x < level.Width; x++)
			{
				float* out = &level.Values[((size_t)y * level.Width + x) * 4];
				float sum[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				for (uint32_t sy = 0; sy <= stepY; sy++)
				{
					for (uint32_t sx = 0; sx <= stepX; sx++)
					{
						const float* in = &source.Values[(((size_t)(y * 2 + sy)) * source.Width + x * 2 + sx) * 4];
						sum[0] += in[0];
						sum[1] += in[1];
						sum[2] += in[2];
						sum[3] += in[3];
					}
				}
				for (int c = 0; c < 4; c++)
				{
					out[c] = sum[c] * weight;
				}
				if (kind == MapKind::Normal)
				{
					const float nx = out[0] * 2.0f - 1.0f, ny = out[1] * 2.0f - 1.0f, nz = out[2] * 2.0f - 1.0f;
					const float length = std::sqrt(nx * nx + ny * ny + nz * nz);
					if (length > 0.0001f)
					{
						out[0] = (nx / length) * 0.5f + 0.5f;
						out[1] = (ny / length) * 0.5f + 0.5f;
						out[2] = (nz / length) * 0.5f + 0.5f;
					}
				}
			}
		}
		return level;
	}

	static uint32_t GetFullMipCount(uint32_t width, uint32_t height)
	{
		uint32_t count = 1;
		uint32_t size = std::max(width, height);
		while (size > 1)
		{
			size /= 2;
			count++;
		}
		return count;
	}

	// The same grid of samples, and the same meaning, as EnvMapVulkanMaterialBuffer::GetAverageColor on a source's pixels
	static glm::vec4 GetAverage(const uint8_t* rgba, uint32_t width, uint32_t height, MapKind kind)
	{
		const float* srgb = GetSrgbToLinearTable();
		const uint32_t stepX = std::max(width / 128u, 1u), stepY = std::max(height / 128u, 1u);
		glm::dvec4 sum(0.0);
		uint64_t samples = 0;
		for (uint32_t y = stepY / 2; y < height; y += stepY)
		{
			for (uint32_t x = stepX / 2; x < width; x += stepX)
			{
				const uint8_t* p = rgba + ((uint64_t)y * width + x) * 4;
				if (kind == MapKind::Color)
				{
					sum += glm::dvec4(srgb[p[0]], srgb[p[1]], srgb[p[2]], p[3] / 255.0f);
				}
				else
				{
					sum += glm::dvec4(p[0] / 255.0f, p[1] / 255.0f, p[2] / 255.0f, p[3] / 255.0f);
				}
				samples++;
			}
		}
		return samples > 0 ? glm::vec4(sum / (double)samples) : glm::vec4(1.0f);
	}

	// ---- Blocks

	static void InitializeEncoders()
	{
		static std::once_flag s_Once;
		std::call_once(s_Once, []()
		{
			bc7enc_compress_block_init();
			rgbcx::init();
		});
	}

	static uint32_t GetEncoderThreads()
	{
		// Half of the machine at most: the editor or the game runs beside it
		return std::min(std::max(std::thread::hardware_concurrency() / 2, 1u), 8u);
	}

	// A level's blocks, each from 4 x 4 pixels; past the image's edge the last row and column repeat. False: stopped
	static bool EncodeLevel(const uint8_t* rgba, uint32_t width, uint32_t height, MapKind kind, std::vector<uint8_t>& out, uint64_t& offset,
		uint32_t threads, const std::atomic<bool>& stop)
	{
		const uint32_t blocksWide = (width + 3) / 4, blocksHigh = (height + 3) / 4;
		bc7enc_compress_block_params params;
		bc7enc_compress_block_params_init(&params);
		if (kind == MapKind::Color)
		{
			bc7enc_compress_block_params_init_perceptual_weights(&params);
		}
		else
		{
			bc7enc_compress_block_params_init_linear_weights(&params);
		}

		std::atomic<uint32_t> nextRow(0);
		uint8_t* blocks = out.data() + offset;
		auto work = [&]()
		{
			uint32_t row;
			while (!stop.load() && (row = nextRow.fetch_add(1)) < blocksHigh)
			{
				for (uint32_t column = 0; column < blocksWide; column++)
				{
					uint8_t block[16 * 4];
					for (uint32_t y = 0; y < 4; y++)
					{
						const uint32_t sourceY = std::min(row * 4 + y, height - 1);
						for (uint32_t x = 0; x < 4; x++)
						{
							const uint32_t sourceX = std::min(column * 4 + x, width - 1);
							memcpy(block + (y * 4 + x) * 4, rgba + ((size_t)sourceY * width + sourceX) * 4, 4);
						}
					}
					uint8_t* target = blocks + ((size_t)row * blocksWide + column) * BlockSize;
					if (kind == MapKind::Normal)
					{
						rgbcx::encode_bc5_hq(target, block, 0, 1, 4);
					}
					else
					{
						bc7enc_compress_block(target, block, &params);
					}
				}
			}
		};

		const uint32_t helpers = blocksHigh > 8 ? std::min(threads, blocksHigh) - 1 : 0;
		std::vector<std::thread> pool;
		pool.reserve(helpers);
		for (uint32_t i = 0; i < helpers; i++)
		{
			pool.emplace_back(work);
		}
		work();
		for (std::thread& thread : pool)
		{
			thread.join();
		}
		offset += (uint64_t)blocksWide * blocksHigh * BlockSize;
		return !stop.load();
	}

	static uint64_t GetLevelBytes(uint32_t width, uint32_t height)
	{
		return (uint64_t)((width + 3) / 4) * ((height + 3) / 4) * BlockSize;
	}

	// A whole image to blocks: levels largest first. False: stopped
	static bool EncodeImage(const uint8_t* rgba, uint32_t width, uint32_t height, MapKind kind, CookedTexture& texture, uint32_t threads, const std::atomic<bool>& stop)
	{
		InitializeEncoders();
		texture.Format = kind == MapKind::Normal ? BlockFormat::BC5 : kind == MapKind::Color ? BlockFormat::BC7_SRGB : BlockFormat::BC7;
		texture.Width = width;
		texture.Height = height;
		texture.Average = GetAverage(rgba, width, height, kind);

		const uint32_t levelCount = GetFullMipCount(width, height);
		texture.Levels.resize(levelCount);
		uint64_t total = 0;
		for (uint32_t i = 0; i < levelCount; i++)
		{
			CookedLevel& level = texture.Levels[i];
			level.Width = std::max(width >> i, 1u);
			level.Height = std::max(height >> i, 1u);
			level.Offset = total;
			level.Size = GetLevelBytes(level.Width, level.Height);
			total += level.Size;
		}
		texture.Bytes.resize((size_t)total);

		uint64_t offset = 0;
		if (!EncodeLevel(rgba, width, height, kind, texture.Bytes, offset, threads, stop))
		{
			return false;
		}
		Pixels pixels = ToPixels(rgba, width, height, kind);
		std::vector<uint8_t> bytes;
		for (uint32_t i = 1; i < levelCount; i++)
		{
			pixels = MakeNextLevel(pixels, kind);
			ToBytes(pixels, kind, bytes);
			if (!EncodeLevel(bytes.data(), pixels.Width, pixels.Height, kind, texture.Bytes, offset, threads, stop))
			{
				return false;
			}
		}
		return true;
	}

	// ---- The file: KTX 2.0 (https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html), readable by other tools. What this
	// cooker needs besides the image is one key-value entry, MoravaCook

	static const uint8_t KtxIdentifier[12] = { 0xAB, 'K', 'T', 'X', ' ', '2', '0', 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };
	static const char CookKey[] = "MoravaCook";

	static uint32_t GetVkFormat(BlockFormat format)
	{
		switch (format)
		{
			case BlockFormat::BC5:      return 141; // VK_FORMAT_BC5_UNORM_BLOCK
			case BlockFormat::BC7:      return 145; // VK_FORMAT_BC7_UNORM_BLOCK
			case BlockFormat::BC7_SRGB: return 146; // VK_FORMAT_BC7_SRGB_BLOCK
		}
		return 0;
	}

	static bool GetFormatFromVk(uint32_t vkFormat, BlockFormat& format)
	{
		switch (vkFormat)
		{
			case 141: format = BlockFormat::BC5;      return true;
			case 145: format = BlockFormat::BC7;      return true;
			case 146: format = BlockFormat::BC7_SRGB; return true;
			default: return false;
		}
	}

	struct CookInfo
	{
		uint32_t Version = 0;
		uint32_t Kind = 0;
		uint64_t SourceSize = 0;
		int64_t SourceWriteTime = 0;
		uint64_t SourceHash = 0;
		uint64_t DataHash = 0;
		float Average[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	};
	static_assert(std::is_trivially_copyable<CookInfo>::value, "written as bytes");

	struct Appender
	{
		std::vector<uint8_t>& Bytes;

		template<typename T>
		void Value(const T& value)
		{
			const uint8_t* start = reinterpret_cast<const uint8_t*>(&value);
			Bytes.insert(Bytes.end(), start, start + sizeof(T));
		}
		void Raw(const void* data, size_t size)
		{
			const uint8_t* start = static_cast<const uint8_t*>(data);
			Bytes.insert(Bytes.end(), start, start + size);
		}
		void Align(size_t alignment)
		{
			while (Bytes.size() % alignment != 0)
			{
				Bytes.push_back(0);
			}
		}
	};

	static void AppendKeyValue(Appender& out, const char* key, const void* value, uint32_t size)
	{
		const uint32_t keyBytes = (uint32_t)strlen(key) + 1;
		out.Value(keyBytes + size);
		out.Raw(key, keyBytes);
		out.Raw(value, size);
		out.Align(4);
	}

	static std::vector<uint8_t> MakeKtx(const CookedTexture& texture, CookInfo info)
	{
		info.Version = CookerVersion;
		info.DataHash = CookFiles::HashBytes(texture.Bytes.data(), texture.Bytes.size());
		memcpy(info.Average, &texture.Average[0], sizeof(info.Average));

		// The data format descriptor: one basic block, which says what the blocks hold
		std::vector<uint8_t> descriptor;
		{
			const bool bc5 = texture.Format == BlockFormat::BC5;
			Appender out{ descriptor };
			const uint32_t sampleCount = bc5 ? 2 : 1;
			const uint16_t blockSize = (uint16_t)(24 + 16 * sampleCount);
			out.Value((uint32_t)(4 + blockSize)); // total size
			out.Value((uint32_t)0);               // vendor Khronos, descriptor type basic
			out.Value((uint16_t)2);               // version
			out.Value(blockSize);
			out.Value((uint8_t)(bc5 ? 131 : 134));                                 // model BC5 or BC7
			out.Value((uint8_t)1);                                                 // primaries BT.709
			out.Value((uint8_t)(texture.Format == BlockFormat::BC7_SRGB ? 2 : 1)); // transfer function sRGB or linear
			out.Value((uint8_t)0);                                                 // flags: alpha straight
			const uint8_t dimensions[4] = { 3, 3, 0, 0 };                          // the block is 4 x 4
			out.Raw(dimensions, 4);
			const uint8_t bytesPlane[8] = { 16, 0, 0, 0, 0, 0, 0, 0 };
			out.Raw(bytesPlane, 8);
			for (uint32_t sample = 0; sample < sampleCount; sample++)
			{
				out.Value((uint16_t)(sample * 64));         // first bit
				out.Value((uint8_t)(bc5 ? 63 : 127));       // length, less one
				out.Value((uint8_t)sample);                 // channel: red, then green (BC5); color (BC7)
				const uint8_t position[4] = { 0, 0, 0, 0 };
				out.Raw(position, 4);
				out.Value((uint32_t)0);
				out.Value((uint32_t)0xFFFFFFFFu);
			}
		}

		// Sorted by key. The rows go from the bottom up, as the engine's images do
		std::vector<uint8_t> keyValues;
		{
			Appender out{ keyValues };
			AppendKeyValue(out, "KTXorientation", "ru", 3);
			AppendKeyValue(out, "KTXwriter", "MoravaEngine", 13);
			AppendKeyValue(out, CookKey, &info, sizeof(info));
		}

		const uint32_t levelCount = (uint32_t)texture.Levels.size();
		const uint32_t headerBytes = 12 + 9 * 4 + 4 * 4 + 2 * 8;
		const uint32_t descriptorOffset = headerBytes + levelCount * 3 * 8;
		const uint32_t keyValueOffset = descriptorOffset + (uint32_t)descriptor.size();

		std::vector<uint8_t> file;
		file.reserve(keyValueOffset + keyValues.size() + 32 + texture.Bytes.size());
		Appender out{ file };
		out.Raw(KtxIdentifier, 12);
		out.Value(GetVkFormat(texture.Format));
		out.Value((uint32_t)1); // type size: compressed
		out.Value(texture.Width);
		out.Value(texture.Height);
		out.Value((uint32_t)0); // depth
		out.Value((uint32_t)0); // layers: not an array
		out.Value((uint32_t)1); // faces
		out.Value(levelCount);
		out.Value((uint32_t)0); // no supercompression
		out.Value(descriptorOffset);
		out.Value((uint32_t)descriptor.size());
		out.Value(keyValueOffset);
		out.Value((uint32_t)keyValues.size());
		out.Value((uint64_t)0); // no supercompression data
		out.Value((uint64_t)0);

		// In the file the smallest level comes first, each on a block's boundary; the index lists the largest first
		uint64_t position = keyValueOffset + keyValues.size();
		position = (position + BlockSize - 1) / BlockSize * BlockSize;
		const uint64_t firstLevel = position;
		std::vector<uint64_t> levelOffsets(levelCount);
		for (uint32_t i = levelCount; i-- > 0;)
		{
			levelOffsets[i] = position;
			position += texture.Levels[i].Size;
		}
		for (uint32_t i = 0; i < levelCount; i++)
		{
			out.Value(levelOffsets[i]);
			out.Value(texture.Levels[i].Size);
			out.Value(texture.Levels[i].Size);
		}
		out.Raw(descriptor.data(), descriptor.size());
		out.Raw(keyValues.data(), keyValues.size());
		file.resize((size_t)firstLevel, 0);
		for (uint32_t i = levelCount; i-- > 0;)
		{
			out.Raw(texture.Bytes.data() + texture.Levels[i].Offset, (size_t)texture.Levels[i].Size);
		}
		return file;
	}

	template<typename T>
	static T ReadAt(const uint8_t* bytes, size_t offset)
	{
		T value;
		memcpy(&value, bytes + offset, sizeof(T));
		return value;
	}

	// Where this cooker's entry is in the file, so that a touched source's time can be brought up to date. Every offset is
	// checked against the file's size: a cut or damaged file reads as false, never past the end
	static bool ParseKtxHeader(const uint8_t* bytes, size_t size, CookedTexture& texture, std::vector<uint64_t>& fileOffsets, CookInfo& info, size_t& infoOffset)
	{
		const size_t headerBytes = 12 + 9 * 4 + 4 * 4 + 2 * 8;
		if (size < headerBytes || memcmp(bytes, KtxIdentifier, 12) != 0)
		{
			return false;
		}
		if (!GetFormatFromVk(ReadAt<uint32_t>(bytes, 12), texture.Format))
		{
			return false;
		}
		texture.Width = ReadAt<uint32_t>(bytes, 20);
		texture.Height = ReadAt<uint32_t>(bytes, 24);
		const uint32_t levelCount = ReadAt<uint32_t>(bytes, 40);
		const uint32_t keyValueOffset = ReadAt<uint32_t>(bytes, 56);
		const uint32_t keyValueBytes = ReadAt<uint32_t>(bytes, 60);
		if (texture.Width == 0 || texture.Height == 0 || texture.Width > 65536 || texture.Height > 65536 ||
			levelCount != GetFullMipCount(texture.Width, texture.Height) || size - headerBytes < (uint64_t)levelCount * 3 * 8 ||
			keyValueOffset > size || keyValueBytes > size - keyValueOffset)
		{
			return false;
		}

		bool found = false;
		size_t position = keyValueOffset;
		const size_t end = (size_t)keyValueOffset + keyValueBytes;
		const size_t keyBytes = sizeof(CookKey);
		while (position + 4 <= end)
		{
			const uint32_t entryBytes = ReadAt<uint32_t>(bytes, position);
			position += 4;
			if (entryBytes > end - position)
			{
				return false;
			}
			if (entryBytes == keyBytes + sizeof(CookInfo) && memcmp(bytes + position, CookKey, keyBytes) == 0)
			{
				infoOffset = position + keyBytes;
				memcpy(&info, bytes + infoOffset, sizeof(CookInfo));
				found = true;
			}
			position += (entryBytes + 3) / 4 * 4;
		}
		if (!found || info.Version != CookerVersion)
		{
			return false;
		}

		memcpy(&texture.Average[0], info.Average, sizeof(info.Average));
		texture.Levels.resize(levelCount);
		fileOffsets.resize(levelCount);
		uint64_t total = 0;
		for (uint32_t i = 0; i < levelCount; i++)
		{
			CookedLevel& level = texture.Levels[i];
			level.Width = std::max(texture.Width >> i, 1u);
			level.Height = std::max(texture.Height >> i, 1u);
			level.Size = GetLevelBytes(level.Width, level.Height);
			level.Offset = total;
			total += level.Size;
			const size_t entry = headerBytes + (size_t)i * 3 * 8;
			fileOffsets[i] = ReadAt<uint64_t>(bytes, entry);
			if (ReadAt<uint64_t>(bytes, entry + 8) != level.Size || fileOffsets[i] > size || level.Size > size - fileOffsets[i])
			{
				return false;
			}
		}
		return true;
	}

	// The levels are copied in the order the data hash covers: largest first
	static bool ParseKtxData(const uint8_t* bytes, const std::vector<uint64_t>& fileOffsets, const CookInfo& info, CookedTexture& texture)
	{
		uint64_t total = 0;
		for (const CookedLevel& level : texture.Levels)
		{
			total += level.Size;
		}
		texture.Bytes.resize((size_t)total);
		for (size_t i = 0; i < texture.Levels.size(); i++)
		{
			memcpy(texture.Bytes.data() + texture.Levels[i].Offset, bytes + fileOffsets[i], (size_t)texture.Levels[i].Size);
		}
		return CookFiles::HashBytes(texture.Bytes.data(), texture.Bytes.size()) == info.DataHash;
	}

	static bool ParseKtx(const uint8_t* bytes, size_t size, CookedTexture& texture, CookInfo& info, size_t& infoOffset)
	{
		std::vector<uint64_t> fileOffsets;
		return ParseKtxHeader(bytes, size, texture, fileOffsets, info, infoOffset) && ParseKtxData(bytes, fileOffsets, info, texture);
	}

	// ---- Reading

	static const char* GetKindName(MapKind kind)
	{
		switch (kind)
		{
			case MapKind::Color:  return "color";
			case MapKind::Normal: return "normal";
			case MapKind::Data:   return "data";
		}
		return "";
	}

	static fs::path GetCookedPath(const std::string& path, MapKind kind)
	{
		return fs::u8path(CookFiles::GetCacheFolder()) / fs::u8path(CookFiles::GetCookedName(path) + "_" + GetKindName(kind) + ".ktx2");
	}

	static std::mutex s_Mutex; // the queue and the numbers
	static Stats s_Stats;

	bool Read(const std::string& path, MapKind kind, CookedTexture& texture, double& milliseconds)
	{
		const auto start = std::chrono::steady_clock::now();
		if (IsCompressionOff(path))
		{
			return false;
		}
		const fs::path cooked = GetCookedPath(path, kind);
		std::vector<uint8_t> bytes;
		if (!CookFiles::ReadWholeFile(cooked, bytes))
		{
			return false;
		}
		CookInfo info;
		size_t infoOffset = 0;
		if (!ParseKtx(bytes.data(), bytes.size(), texture, info, infoOffset))
		{
			Log::GetLogger()->warn("Cooked texture '{0}' is damaged or of an older cooker: the map is cooked again", cooked.generic_u8string());
			return false;
		}
		if (info.Kind != (uint32_t)kind)
		{
			return false;
		}

		// By size and write time, else by content: a Git checkout gives an unchanged file a new time
		const fs::path source = fs::u8path(path);
		uint64_t size = 0;
		int64_t writeTime = 0;
		if (!CookFiles::GetFileState(source, size, writeTime) || size != info.SourceSize)
		{
			return false;
		}
		if (writeTime != info.SourceWriteTime)
		{
			std::vector<uint8_t> sourceBytes;
			if (!CookFiles::ReadWholeFile(source, sourceBytes) || CookFiles::HashBytes(sourceBytes.data(), sourceBytes.size()) != info.SourceHash)
			{
				return false;
			}
			info.SourceWriteTime = writeTime;
			memcpy(bytes.data() + infoOffset, &info, sizeof(info));
			CookFiles::WriteFileSafely(cooked, bytes.data(), bytes.size());
		}

		milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
		std::lock_guard<std::mutex> lock(s_Mutex);
		s_Stats.Read++;
		s_Stats.ReadMilliseconds += milliseconds;
		return true;
	}

	// ---- The cooking thread

	struct Job
	{
		std::string Path;
		MapKind Kind = MapKind::Data;
		fs::path Cooked;
	};

	static std::deque<Job> s_Queue;
	static std::set<std::string> s_Requested; // each file and use once in a session: a failed one isn't tried again until the next
	static std::thread s_Thread;
	static std::condition_variable s_Wake;
	static std::atomic<bool> s_Stop(false);
	static bool s_SummaryLogged = false;

	static void CookOne(const Job& job)
	{
		const auto start = std::chrono::steady_clock::now();
		const fs::path source = fs::u8path(job.Path);
		CookInfo info;
		info.Kind = (uint32_t)job.Kind;

		std::vector<uint8_t> sourceBytes;
		bool ok = CookFiles::GetFileState(source, info.SourceSize, info.SourceWriteTime) && CookFiles::ReadWholeFile(source, sourceBytes);
		int width = 0, height = 0, channels = 0;
		stbi_uc* rgba = nullptr;
		if (ok)
		{
			info.SourceHash = CookFiles::HashBytes(sourceBytes.data(), sourceBytes.size());
			// This thread's setting only (the main thread loads images at the same time). Bottom up, as the engine's images load
			stbi_set_flip_vertically_on_load_thread(1);
			rgba = stbi_load_from_memory(sourceBytes.data(), (int)sourceBytes.size(), &width, &height, &channels, 4);
			ok = rgba != nullptr;
		}
		if (!ok)
		{
			Log::GetLogger()->warn("Texture '{0}' could not be cooked: it can't be read", job.Path);
			std::lock_guard<std::mutex> lock(s_Mutex);
			s_Stats.Failed++;
			return;
		}
		std::vector<uint8_t>().swap(sourceBytes);

		CookedTexture texture;
		const bool finished = EncodeImage(rgba, (uint32_t)width, (uint32_t)height, job.Kind, texture, GetEncoderThreads(), s_Stop);
		stbi_image_free(rgba);
		if (!finished)
		{
			return;
		}

		const std::vector<uint8_t> file = MakeKtx(texture, info);
		if (!CookFiles::WriteFileSafely(job.Cooked, file.data(), file.size()))
		{
			Log::GetLogger()->warn("Cooked texture '{0}' could not be written", job.Cooked.generic_u8string());
			std::lock_guard<std::mutex> lock(s_Mutex);
			s_Stats.Failed++;
			return;
		}

		const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
		Log::GetLogger()->info("Texture '{0}' cooked as {1}: {2} x {3}, {4} levels, {5:.2f} MB, in {6:.1f} s", job.Path, job.Kind == MapKind::Normal ? "BC5" : "BC7",
			width, height, texture.Levels.size(), texture.Bytes.size() / (1024.0 * 1024.0), seconds);
		std::lock_guard<std::mutex> lock(s_Mutex);
		s_Stats.Cooked++;
		s_Stats.CookSeconds += seconds;
	}

	static void RunCookingThread()
	{
#if defined(_WIN32)
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif
		std::unique_lock<std::mutex> lock(s_Mutex);
		while (true)
		{
			s_Wake.wait(lock, []() { return s_Stop.load() || !s_Queue.empty(); });
			if (s_Stop.load())
			{
				return;
			}
			const Job job = s_Queue.front();
			s_Queue.pop_front();
			s_Stats.Cooking = job.Path;
			lock.unlock();

			CookOne(job);

			lock.lock();
			s_Stats.Cooking.clear();
		}
	}

	// A program that ends some other way than through Application::OnShutdown still stops the thread (a running one would end it badly)
	struct ThreadStopper
	{
		~ThreadStopper() { Shutdown(); }
	};
	static ThreadStopper s_ThreadStopper;

	void Request(const std::string& path, MapKind kind)
	{
		if (IsCompressionOff(path))
		{
			return;
		}
		Job job;
		job.Path = path;
		job.Kind = kind;
		job.Cooked = GetCookedPath(path, kind); // here: on the main thread, for the asset registry and the open project

		std::lock_guard<std::mutex> lock(s_Mutex);
		if (s_Stop.load() || !s_Requested.insert(job.Cooked.generic_u8string()).second)
		{
			return;
		}
		s_Queue.push_back(std::move(job));
		if (!s_Thread.joinable())
		{
			s_Thread = std::thread(RunCookingThread);
		}
		s_Wake.notify_one();
	}

	void Shutdown()
	{
		{
			std::lock_guard<std::mutex> lock(s_Mutex);
			s_Stop.store(true);
			s_Queue.clear();
		}
		s_Wake.notify_all();
		if (s_Thread.joinable())
		{
			s_Thread.join();
		}

		std::lock_guard<std::mutex> lock(s_Mutex);
		if (s_Stats.Read + s_Stats.FromSource > 0 && !s_SummaryLogged)
		{
			s_SummaryLogged = true;
			Log::GetLogger()->info("Material maps: {0} read cooked in {1:.0f} ms, {2} from their images in {3:.0f} ms, {4} cooked in {5:.1f} s, {6} failed",
				s_Stats.Read, s_Stats.ReadMilliseconds, s_Stats.FromSource, s_Stats.SourceMilliseconds, s_Stats.Cooked, s_Stats.CookSeconds, s_Stats.Failed);
		}
	}

	// ---- Settings

	bool IsCompressionOff(const std::string& path)
	{
		const fs::path sidecar = fs::u8path(path + ".meta");
		std::error_code error;
		if (!fs::is_regular_file(sidecar, error))
		{
			return false;
		}
		try
		{
			const YAML::Node settings = YAML::LoadFile(sidecar.u8string())["Import"];
			return settings && settings["Compression"] && settings["Compression"].as<std::string>() == "None";
		}
		catch (const std::exception&)
		{
			return false; // the registry reports a sidecar it can't read
		}
	}

	void SetCompressionOff(const std::string& path, bool off)
	{
		const fs::path sidecar = fs::u8path(path + ".meta");
		try
		{
			YAML::Node root;
			std::error_code error;
			if (fs::is_regular_file(sidecar, error))
			{
				root = YAML::LoadFile(sidecar.u8string());
			}
			if (!root["Asset"])
			{
				Log::GetLogger()->warn("Texture '{0}' has no sidecar: its compression can't be changed", path);
				return;
			}
			if (off)
			{
				root["Import"]["Compression"] = "None";
			}
			else
			{
				root.remove("Import");
			}
			YAML::Emitter out;
			out << root;
			std::ofstream file(sidecar, std::ios::trunc);
			file << out.c_str() << "\n";
		}
		catch (const std::exception& exception)
		{
			Log::GetLogger()->warn("Texture '{0}': its sidecar could not be changed ({1})", path, exception.what());
		}
	}

	// ---- Numbers

	Stats GetStats()
	{
		std::lock_guard<std::mutex> lock(s_Mutex);
		Stats stats = s_Stats;
		stats.Waiting = (uint32_t)s_Queue.size() + (stats.Cooking.empty() ? 0u : 1u);
		return stats;
	}

	void AddSourceLoad(double milliseconds)
	{
		std::lock_guard<std::mutex> lock(s_Mutex);
		s_Stats.FromSource++;
		s_Stats.SourceMilliseconds += milliseconds;
	}

	void CountCachedFiles(uint32_t& files, uint64_t& bytes)
	{
		CookFiles::CountFiles(".ktx2", files, bytes);
	}

	uint32_t ClearCache()
	{
		// What is removed can be cooked again in this session
		{
			std::lock_guard<std::mutex> lock(s_Mutex);
			s_Requested.clear();
		}
		return CookFiles::RemoveFiles(".ktx2");
	}

	// ---- Self-test

	static double GetMeanError(const uint8_t* a, const uint8_t* b, size_t pixels, int channels)
	{
		double sum = 0.0;
		for (size_t i = 0; i < pixels; i++)
		{
			for (int c = 0; c < channels; c++)
			{
				sum += std::abs((int)a[i * 4 + c] - (int)b[i * 4 + c]);
			}
		}
		return sum / (pixels * channels);
	}

	// Mip 0 back to pixels
	static void DecodeFirstLevel(const CookedTexture& texture, std::vector<uint8_t>& rgba)
	{
		const uint32_t width = texture.Width, height = texture.Height;
		rgba.assign((size_t)width * height * 4, 0);
		const uint32_t blocksWide = (width + 3) / 4, blocksHigh = (height + 3) / 4;
		for (uint32_t row = 0; row < blocksHigh; row++)
		{
			for (uint32_t column = 0; column < blocksWide; column++)
			{
				const uint8_t* block = texture.Bytes.data() + ((size_t)row * blocksWide + column) * BlockSize;
				uint8_t decoded[16 * 4] = {};
				if (texture.Format == BlockFormat::BC5)
				{
					rgbcx::unpack_bc5(block, decoded, 0, 1, 4);
				}
				else
				{
					bc7decomp::unpack_bc7(block, reinterpret_cast<bc7decomp::color_rgba*>(decoded));
				}
				for (uint32_t y = 0; y < 4 && row * 4 + y < height; y++)
				{
					for (uint32_t x = 0; x < 4 && column * 4 + x < width; x++)
					{
						memcpy(&rgba[(((size_t)row * 4 + y) * width + column * 4 + x) * 4], decoded + (y * 4 + x) * 4, 4);
					}
				}
			}
		}
	}

	bool SelfTest()
	{
		auto fail = [](const char* what)
		{
			Log::GetLogger()->error("Cooked texture self-test failed: {0}", what);
			return false;
		};

		// Gradients with a soft edge, not a power of two
		const uint32_t width = 70, height = 45;
		std::vector<uint8_t> image((size_t)width * height * 4);
		for (uint32_t y = 0; y < height; y++)
		{
			for (uint32_t x = 0; x < width; x++)
			{
				uint8_t* p = &image[((size_t)y * width + x) * 4];
				p[0] = (uint8_t)(x * 255 / (width - 1));
				p[1] = (uint8_t)(y * 255 / (height - 1));
				p[2] = (uint8_t)(255 - p[0] / 2);
				p[3] = (uint8_t)(x < width / 2 ? 255 : 128);
			}
		}

		// Each kind: the right format, every level, and close to the image once decoded (a test's numbers, far from a good
		// encoder's: it fails on a broken one, not on a small change of quality)
		std::atomic<bool> running(false);
		for (MapKind kind : { MapKind::Color, MapKind::Data, MapKind::Normal })
		{
			CookedTexture texture;
			if (!EncodeImage(image.data(), width, height, kind, texture, 1, running))
			{
				return fail("an image is encoded");
			}
			if (texture.Levels.size() != 7 || texture.Levels.back().Width != 1 || texture.Levels.back().Height != 1)
			{
				return fail("the levels go down to 1 x 1");
			}
			if (texture.Format != (kind == MapKind::Normal ? BlockFormat::BC5 : kind == MapKind::Color ? BlockFormat::BC7_SRGB : BlockFormat::BC7))
			{
				return fail("the format follows the use");
			}
			std::vector<uint8_t> decoded;
			DecodeFirstLevel(texture, decoded);
			if (GetMeanError(image.data(), decoded.data(), (size_t)width * height, kind == MapKind::Normal ? 2 : 4) > 3.0)
			{
				return fail("a decoded image is close to the source");
			}

			CookInfo info;
			info.Kind = (uint32_t)kind;
			info.SourceSize = 7;
			info.SourceWriteTime = 8;
			info.SourceHash = 9;
			const std::vector<uint8_t> file = MakeKtx(texture, info);

			CookedTexture read;
			CookInfo readInfo;
			size_t infoOffset = 0;
			if (!ParseKtx(file.data(), file.size(), read, readInfo, infoOffset))
			{
				return fail("a whole file is read");
			}
			if (read.Bytes != texture.Bytes || read.Format != texture.Format || read.Width != width || read.Height != height || read.Average != texture.Average ||
				readInfo.Kind != info.Kind || readInfo.SourceHash != 9 || readInfo.SourceWriteTime != 8)
			{
				return fail("a file reads back as written");
			}
			if (ReadAt<uint64_t>(file.data(), 12 + 9 * 4 + 4 * 4 + 2 * 8) % BlockSize != 0)
			{
				return fail("levels start on a block's boundary");
			}

			// Cut anywhere, it isn't read; nor with one bit of its data changed
			for (size_t size = 0; size < file.size(); size += 97)
			{
				CookedTexture cut;
				CookInfo cutInfo;
				size_t cutOffset = 0;
				if (ParseKtx(file.data(), size, cut, cutInfo, cutOffset))
				{
					return fail("a cut file is refused");
				}
			}
			std::vector<uint8_t> damaged = file;
			damaged[damaged.size() - 5] ^= 0x10;
			CookedTexture damagedTexture;
			CookInfo damagedInfo;
			size_t damagedOffset = 0;
			if (ParseKtx(damaged.data(), damaged.size(), damagedTexture, damagedInfo, damagedOffset))
			{
				return fail("a damaged file is refused");
			}
		}

		// Averaging happens in linear light for color: black and white make about 188, not 128. For data they make 128
		const uint8_t checker[16] = { 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 255 };
		std::vector<uint8_t> bytes;
		ToBytes(MakeNextLevel(ToPixels(checker, 2, 2, MapKind::Color), MapKind::Color), MapKind::Color, bytes);
		if (bytes[0] < 186 || bytes[0] > 190 || bytes[3] != 255)
		{
			return fail("a color map's mips are averaged in linear light");
		}
		ToBytes(MakeNextLevel(ToPixels(checker, 2, 2, MapKind::Data), MapKind::Data), MapKind::Data, bytes);
		if (bytes[0] < 127 || bytes[0] > 128)
		{
			return fail("a data map's mips are averaged as they are");
		}

		// A normal map's mip is unit length again: (1, 0, 0) and (0, 1, 0) average to 45 degrees, not to a shorter vector
		const uint8_t normals[8] = { 255, 128, 128, 255, 128, 255, 128, 255 };
		const Pixels normalLevel = MakeNextLevel(ToPixels(normals, 2, 1, MapKind::Normal), MapKind::Normal);
		const float nx = normalLevel.Values[0] * 2.0f - 1.0f, ny = normalLevel.Values[1] * 2.0f - 1.0f, nz = normalLevel.Values[2] * 2.0f - 1.0f;
		if (std::abs(nx * nx + ny * ny + nz * nz - 1.0f) > 0.01f)
		{
			return fail("a normal map's mips are unit length");
		}
		return true;
	}
}
