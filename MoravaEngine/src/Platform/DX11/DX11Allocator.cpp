#include "DX11Allocator.h"

#include "DX11Context.h"


DX11Allocator::DX11Allocator(const Hazel::Ref<DX11Device>& device, const std::string& tag)
	: m_Device(device), m_Tag(tag)
{
}

DX11Allocator::DX11Allocator(const std::string& tag)
	: m_Device(DX11Context::GetCurrentDevice()), m_Tag(tag)
{
}

DX11Allocator::~DX11Allocator()
{
}

void DX11Allocator::Allocate()
{
}