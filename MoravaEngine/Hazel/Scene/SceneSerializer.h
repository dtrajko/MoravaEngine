#pragma once

#include "HazelScene.h"


namespace Hazel {

	class SceneSerializer
	{
	public:
		SceneSerializer(HazelScene* scene);

		void Serialize(const std::string& filepath);
		void SerializeRuntime(const std::string &filepath);

		bool Deserialize(const std::string& filepath);
		bool DeserializeRuntime(const std::string& filepath);

	private:
		HazelScene* m_Scene;

	};

}
