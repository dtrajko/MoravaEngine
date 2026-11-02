#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"
#include "DX11Device.h"

#include <string>


class DX11Allocator
{
public:
	DX11Allocator() = default;
	DX11Allocator(const std::string& tag);
	DX11Allocator(const H2M::RefH2M<DX11Device>& device, const std::string& tag = "");
	~DX11Allocator();

	void Allocate();

private:
	H2M::RefH2M<DX11Device> m_Device;
	std::string m_Tag;
};
