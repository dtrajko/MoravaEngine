/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include <string>


namespace H2M {

	class HashH2M
	{
	public:
		static uint32_t GenerateFNVHash(const char* str);
		static uint32_t GenerateFNVHash(const std::string& string);

		static uint32_t CRC32(const char* str);
		static uint32_t CRC32(const std::string& string);
	};

}
