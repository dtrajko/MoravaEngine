#include "DX11Allocator.h"

#include "DX11Context.h"


DX11Allocator::DX11Allocator(const H2M::RefH2M<DX11Device>& device, const std::string& tag)
	: m_Device(device), m_Tag(tag)
{
}

DX11Allocator::DX11Allocator(const std::string& tag)
	: m_Device(DX11Context::Get()->GetCurrentDevice()), m_Tag(tag)
{
}

DX11Allocator::~DX11Allocator()
{
}

void DX11Allocator::Allocate()
{
}
