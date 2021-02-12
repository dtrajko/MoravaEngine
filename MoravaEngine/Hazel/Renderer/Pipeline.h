#pragma once

#include "../Core/Ref.h"
#include "HazelShader.h"


namespace Hazel {

	struct PipelineSpecification
	{
		Ref<HazelShader> Shader;
		VertexBufferLayout Layout;

	};


	class Pipeline
	{
	public:
		virtual ~Pipeline() = default;

		virtual PipelineSpecification& GetSpecification() = 0;
		virtual const PipelineSpecification& GetSpecification() const = 0;

		static Ref<Pipeline> Create(const PipelineSpecification& spec);

	private:


	};

}
