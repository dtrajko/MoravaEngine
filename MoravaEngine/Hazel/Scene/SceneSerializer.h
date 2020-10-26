#pragma once

#include "HazelScene.h"


namespace Hazel {

	class SceneSerializer
	{
	public:
		SceneSerializer(const Ref<HazelScene>& scene);

		void Serialize(const std::string& filepath);
		void SerializeRuntime(const std::string &filepath);

		bool Deserialize(const std::string& filepath);
		bool DeserializeRuntime(const std::string& filepath);

	private:
		Ref<HazelScene> m_Scene;
	};

}
