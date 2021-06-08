#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Ref.h"

#include "DX11.h"

#include <unordered_set>

	
class DX11PhysicalDevice : public Hazel::RefCounted
{
public:
	DX11PhysicalDevice();
	~DX11PhysicalDevice();

	static Hazel::Ref<DX11PhysicalDevice> Select();

	friend class DX11Device;
};

// Represents a logical device
class DX11Device : public Hazel::RefCounted
{
public:
	DX11Device(const Hazel::Ref<DX11PhysicalDevice>& physicalDevice);
	~DX11Device();

	const Hazel::Ref<DX11PhysicalDevice>& GetPhysicalDevice() const { return m_PhysicalDevice; }

private:
	Hazel::Ref<DX11PhysicalDevice> m_PhysicalDevice;

};
