/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "EntityH2M.h"


namespace H2M
{

	class ScriptableEntityH2M
	{
	public:
		virtual ~ScriptableEntityH2M() {}

		template<typename T>
		T& GetComponent()
		{
			return m_Entity.GetComponent<T>();
		}
	protected:
		virtual void OnCreate() {}
		virtual void OnDestroy() {}
		virtual void OnUpdate(TimestepH2M ts) {}

	private:
		EntityH2M m_Entity;
		friend class SceneH2M;
	};

}
