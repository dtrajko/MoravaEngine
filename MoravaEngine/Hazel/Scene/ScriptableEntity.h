#pragma once

#include "Entity.h"


namespace Hazel
{
	class Entity;

	class ScriptableEntity
	{
	public:
		virtual ~ScriptableEntity() {};

		template<typename T>
		T& GetComponent()
		{
			return m_Entity.GetComponent<T>();
		}

	protected:
		virtual void OnCreate() {}
		virtual void OnDestroy() {}
		virtual void OnUpdate(float ts) {}

	private:
		Entity m_Entity;

		friend class Scene;
		friend class HazelScene; // only temporary

	};
}
