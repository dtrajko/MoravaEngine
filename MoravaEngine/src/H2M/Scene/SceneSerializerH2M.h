/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "SceneH2M.h"


namespace H2M
{

	class SceneSerializerH2M
	{
	public:
		SceneSerializerH2M(const RefH2M<SceneH2M>& scene);

		void Serialize(const std::string& filepath);
		void SerializeRuntime(const std::string& filepath);

		bool Deserialize(const std::string& filepath);
		bool DeserializeRuntime(const std::string& filepath);

	private:
		RefH2M<SceneH2M> m_Scene;

	};

}
