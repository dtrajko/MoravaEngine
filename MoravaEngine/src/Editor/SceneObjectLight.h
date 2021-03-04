#pragma once

#include "Editor/SceneObject.h"


class SceneObjectLight : public SceneObject
{
public:
	SceneObjectLight();
	~SceneObjectLight();

	inline std::string GetType() { return m_Type; };
	inline unsigned int GetIndex() { return m_Index; };

private:
	std::string m_Type; // directional, point, light
	unsigned int m_Index; // directional: 0, point: 0-3, spot: 0-3
};
