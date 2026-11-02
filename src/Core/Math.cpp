#include "Core/Math.h"

#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <time.h>


bool Math::s_IsRandomInit = false;

// Rotation units are Radians
glm::mat4 Math::CreateTransformHazel(glm::vec3 Translation, glm::vec3 Rotation, glm::vec3 Scale)
{
	glm::mat4 rotation =
		glm::rotate(glm::mat4(1.0f), Rotation.x, { 1, 0, 0 }) *
		glm::rotate(glm::mat4(1.0f), Rotation.y, { 0, 1, 0 }) *
		glm::rotate(glm::mat4(1.0f), Rotation.z, { 0, 0, 1 });

	return glm::translate(glm::mat4(1.0f), Translation) *
		rotation *
		glm::scale(glm::mat4(1.0f), Scale);
}

glm::mat4 Math::CreateTransform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
{
	glm::mat4 transform = glm::mat4(1.0f);
	transform *= glm::translate(glm::mat4(1.0f), position);
	transform *= glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	transform *= glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	transform *= glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	transform *= glm::scale(glm::mat4(1.0f), scale);
	return transform;
}

glm::mat4 Math::CreateTransform(glm::vec3 position, glm::quat rotation, glm::vec3 scale)
{
	glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0f), position);
	glm::mat4 rotateMatrix    = glm::toMat4(rotation);
	glm::mat4 scaleMatrix     = glm::scale(glm::mat4(1.0f), scale);
	return translateMatrix * rotateMatrix * scaleMatrix;
}

/**
 * Same as Math::Mat4FromAssimpMat4
*/
glm::mat4 Math::aiMatrix4x4ToGlm(const aiMatrix4x4* from)
{
    glm::mat4 to;

    to[0][0] = (float)from->a1; to[0][1] = (float)from->b1;  to[0][2] = (float)from->c1; to[0][3] = (float)from->d1;
    to[1][0] = (float)from->a2; to[1][1] = (float)from->b2;  to[1][2] = (float)from->c2; to[1][3] = (float)from->d2;
    to[2][0] = (float)from->a3; to[2][1] = (float)from->b3;  to[2][2] = (float)from->c3; to[2][3] = (float)from->d3;
    to[3][0] = (float)from->a4; to[3][1] = (float)from->b4;  to[3][2] = (float)from->c4; to[3][3] = (float)from->d4;

    return to;
}

glm::mat4 Math::Mat4FromAssimpMat4(const aiMatrix4x4& matrix)
{
	glm::mat4 result;
	//the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
	result[0][0] = matrix.a1; result[1][0] = matrix.a2; result[2][0] = matrix.a3; result[3][0] = matrix.a4;
	result[0][1] = matrix.b1; result[1][1] = matrix.b2; result[2][1] = matrix.b3; result[3][1] = matrix.b4;
	result[0][2] = matrix.c1; result[1][2] = matrix.c2; result[2][2] = matrix.c3; result[3][2] = matrix.c4;
	result[0][3] = matrix.d1; result[1][3] = matrix.d2; result[2][3] = matrix.d3; result[3][3] = matrix.d4;
	return result;
}

float Math::ConvertRangeFloat(float value, float oldMin, float oldMax, float newMin, float newMax)
{
	return (((value - oldMin) * (newMax - newMin)) / (oldMax - oldMin)) + newMin;
}

float Math::Lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

/**
 * Convert an arbitrary range to [0-1] range
 */
float Math::InverseLerp(float xx, float yy, float value)
{
    return (value - xx) / (yy - xx);
}

std::tuple<glm::vec3, glm::quat, glm::vec3> Math::GetTransformDecomposition(const glm::mat4& transform)
{
	glm::vec3 scale, translation, skew;
	glm::vec4 perspective;
	glm::quat orientation;

	glm::decompose(transform, scale, orientation, translation, skew, perspective);

	return { translation, orientation, scale };
}

bool Math::DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale)
{
    // From glm::decompose in matrix_decompose.inl

    using namespace glm;
    using T = float;

    mat4 LocalMatrix(transform);

    // Normalize the matrix
    if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>())) {
        return false;
    }

    // First, isolate perspective. This is the messiest.
    if (
        epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
        epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
        epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
    {
        // Clear the perspective partition
        LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
        LocalMatrix[3][3] = static_cast<T>(1);
    }

    // Next take care of translation (easy).
    translation = vec3(LocalMatrix[3]);
    LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

    vec3 Row[3];

    // Now get scale and shear.
    for (length_t i = 0; i < 3; ++i)
        for (length_t j = 0; j < 3; ++j)
            Row[i][j] = LocalMatrix[i][j];

    // Compute X scale factor and normalize first row.
    scale.x = length(Row[0]);
    Row[0] = detail::scale(Row[0], static_cast<T>(1));
    scale.y = length(Row[1]);
    Row[1] = detail::scale(Row[1], static_cast<T>(1));
    scale.z = length(Row[2]);
    Row[2] = detail::scale(Row[2], static_cast<T>(1));

    // At this point, the matrix (in rows[]) is orthonormal.
    // Check for a coordinate system flip. If the determinant
    // is -1, the negate the matrix and the scaling factors.

#if 0
    vec3 Pdum3;
    Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
    if (dot(Row[0], Pdum3) < 0)
    {
        for (length_t i = 0; i < 3; i++)
        {
            scale[i] *= static_cast<T>(-1);
            Row[i] *= static_cast<T>(-1);
        }
    }
#endif

    rotation.y = asin(-Row[0][2]);
    if (cos(rotation.y) != 0) {
        rotation.x = atan2(Row[1][2], Row[2][2]);
        rotation.z = atan2(Row[0][1], Row[0][0]);
    }
    else {
        rotation.x = atan2(-Row[2][0], Row[1][1]);
        rotation.z = 0;
    }

    return true;
}
