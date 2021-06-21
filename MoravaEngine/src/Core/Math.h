#pragma once

#include "Core/Random.h"

#include <glm/glm.hpp>

#include <assimp/scene.h>


class Math
{
public:
	static glm::mat4 CreateTransformHazel(glm::vec3 Translation, glm::vec3 Rotation, glm::vec3 Scale);
	static glm::mat4 CreateTransform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);
	static glm::mat4 CreateTransform(glm::vec3 position, glm::quat rotation, glm::vec3 scale);
    static glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from);
	static glm::mat4 Mat4FromAssimpMat4(const aiMatrix4x4& matrix);

	static float Lerp(float a, float b, float f);
	static float InverseLerp(float xx, float yy, float value);

	static float ConvertRangeFloat(float value, float oldMin, float oldMax, float newMin, float newMax);

	static std::tuple<glm::vec3, glm::quat, glm::vec3> GetTransformDecomposition(const glm::mat4& transform);
	static bool DecomposeTransform(const glm::mat4& transform, glm::vec3& outTranslation, glm::vec3& outRotation, glm::vec3& outScale);

private:
	static bool s_IsRandomInit;

};
