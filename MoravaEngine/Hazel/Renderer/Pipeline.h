#pragma once

#include "../Core/Ref.h"


namespace Hazel {

	struct PipelineSpecification
	{


	};


	class Pipeline
	{
	public:
		static Ref<Pipeline> Create(const PipelineSpecification& spec);

	private:


	};

}
