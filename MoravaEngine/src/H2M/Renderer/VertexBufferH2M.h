/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/BaseH2M.h"
#include "H2M/Core/AssertH2M.h"
#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/RendererTypesH2M.h"

#include "Core/Log.h"

#include <string>
#include <vector>


namespace H2M {

	enum class ShaderDataTypeH2M
	{
		None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool
	};

	static uint32_t ShaderDataTypeSize(ShaderDataTypeH2M type)
	{
		switch (type)
		{
		case ShaderDataTypeH2M::Float:    return 4;
		case ShaderDataTypeH2M::Float2:   return 4 * 2;
		case ShaderDataTypeH2M::Float3:   return 4 * 3;
		case ShaderDataTypeH2M::Float4:   return 4 * 4;
		case ShaderDataTypeH2M::Mat3:     return 4 * 3 * 3;
		case ShaderDataTypeH2M::Mat4:     return 4 * 4 * 4;
		case ShaderDataTypeH2M::Int:      return 4;
		case ShaderDataTypeH2M::Int2:     return 4 * 2;
		case ShaderDataTypeH2M::Int3:     return 4 * 3;
		case ShaderDataTypeH2M::Int4:     return 4 * 4;
		case ShaderDataTypeH2M::Bool:     return 1;
		}

		H2M_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}

	struct VertexBufferElementH2M
	{
		std::string Name;
		ShaderDataTypeH2M Type;
		uint32_t Size;
		uint32_t Offset;
		bool Normalized;

		VertexBufferElementH2M() = default;

		VertexBufferElementH2M(ShaderDataTypeH2M type, const std::string& name, bool normalized = false)
			: Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized)
		{
		}

		uint32_t GetComponentCount() const
		{
			switch (Type)
			{
			case ShaderDataTypeH2M::Float:   return 1;
			case ShaderDataTypeH2M::Float2:  return 2;
			case ShaderDataTypeH2M::Float3:  return 3;
			case ShaderDataTypeH2M::Float4:  return 4;
			case ShaderDataTypeH2M::Mat3:    return 3 * 3;
			case ShaderDataTypeH2M::Mat4:    return 4 * 4;
			case ShaderDataTypeH2M::Int:     return 1;
			case ShaderDataTypeH2M::Int2:    return 2;
			case ShaderDataTypeH2M::Int3:    return 3;
			case ShaderDataTypeH2M::Int4:    return 4;
			case ShaderDataTypeH2M::Bool:    return 1;
			}

			H2M_CORE_ASSERT(false, "Unknown ShaderDataType!");
			return 0;
		}
	};

	class VertexBufferLayoutH2M
	{
	public:
		VertexBufferLayoutH2M() {}

		VertexBufferLayoutH2M(const std::initializer_list<VertexBufferElementH2M>& elements)
			: m_Elements(elements)
		{
			CalculateOffsetsAndStride();
		}

		inline uint32_t GetStride() const { return m_Stride; }
		inline const std::vector<VertexBufferElementH2M>& GetElements() const { return m_Elements; }
		uint32_t GetElementCount() const { return (uint32_t)m_Elements.size(); }

		std::vector<VertexBufferElementH2M>::iterator begin() { return m_Elements.begin(); }
		std::vector<VertexBufferElementH2M>::iterator end() { return m_Elements.end(); }
		std::vector<VertexBufferElementH2M>::const_iterator begin() const { return m_Elements.begin(); }
		std::vector<VertexBufferElementH2M>::const_iterator end() const { return m_Elements.end(); }
	private:
		void CalculateOffsetsAndStride()
		{
			uint32_t offset = 0;
			m_Stride = 0;
			for (auto& element : m_Elements)
			{
				element.Offset = offset;
				offset += element.Size;
				m_Stride += element.Size;
			}
		}
	private:
		std::vector<VertexBufferElementH2M> m_Elements;
		uint32_t m_Stride = 0;
	};

	enum class VertexBufferUsageH2M
	{
		None = 0, Static = 1, Dynamic = 2
	};

	class VertexBufferH2M : public RefCountedH2M
	{
	public:
		virtual ~VertexBufferH2M() {}

		virtual void SetData(void* buffer, uint32_t size, uint32_t offset = 0) = 0;
		virtual void RT_SetData(void* buffer, uint32_t size, uint32_t offset = 0) = 0;
		virtual void Bind() const = 0;

		virtual uint32_t GetSize() const = 0;
		virtual RendererID_H2M GetRendererID() const = 0;

		virtual const VertexBufferLayoutH2M& GetLayout() const = 0;
		virtual void SetLayout(const VertexBufferLayoutH2M& layout) = 0;

		static RefH2M<VertexBufferH2M> Create(void* data, uint32_t size, VertexBufferUsageH2M usage = VertexBufferUsageH2M::Static);
		static RefH2M<VertexBufferH2M> Create(uint32_t size, VertexBufferUsageH2M usage = VertexBufferUsageH2M::Dynamic);
		// DirectX 11 vertex buffer
		static RefH2M<VertexBufferH2M> Create(void* data, uint32_t stride, uint32_t count, VertexBufferUsageH2M usage = VertexBufferUsageH2M::Static);

	};

}
