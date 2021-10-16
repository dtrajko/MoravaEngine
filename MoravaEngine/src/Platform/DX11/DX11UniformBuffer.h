#pragma once

#include "Hazel/Renderer/UniformBuffer.h"

#include <string>


namespace Hazel
{

	class DX11UniformBuffer : public UniformBuffer
	{
	public:
		DX11UniformBuffer(uint32_t size, uint32_t binding);
		~DX11UniformBuffer();

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
		virtual void RT_SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
		virtual uint32_t GetBinding() const override { return m_Binding; }

	private:
		void Release();
		void RT_Invalidate();

	private:
		uint32_t m_Size = 0;
		uint32_t m_Binding = 0;
		std::string m_Name;

		uint32_t* m_LocalStorage = nullptr;
	};

}
