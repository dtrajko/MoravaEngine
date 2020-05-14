#include "AABB.h"

#include "Math.h"

#include <limits>


AABB::AABB(glm::vec3 position, glm::vec3 rotation, glm::vec3 scaleObject)
{
	m_Position = position;
    m_Rotation = rotation;
	m_ScaleObject = scaleObject;
    m_ScaleAABB = scaleObject;

    glm::vec3 size = glm::vec3(
        m_UnitSize * scaleObject.x + m_Offset,
        m_UnitSize * scaleObject.y + m_Offset,
        m_UnitSize * scaleObject.z + m_Offset);

    m_OriginVectors.push_back(glm::vec3(m_Position.x - size.x, m_Position.y - size.y, m_Position.z + size.z)); // 0 - MinX MinY MaxZ
    m_OriginVectors.push_back(glm::vec3(m_Position.x + size.x, m_Position.y - size.y, m_Position.z + size.z)); // 1 - MaxX MinY MaxZ
    m_OriginVectors.push_back(glm::vec3(m_Position.x + size.x, m_Position.y + size.y, m_Position.z + size.z)); // 2 - MaxX MaxY MaxZ
    m_OriginVectors.push_back(glm::vec3(m_Position.x - size.x, m_Position.y + size.y, m_Position.z + size.z)); // 3 - MinX MaxY MaxZ
    m_OriginVectors.push_back(glm::vec3(m_Position.x - size.x, m_Position.y - size.y, m_Position.z - size.z)); // 4 - MinX MinY MinZ
    m_OriginVectors.push_back(glm::vec3(m_Position.x + size.x, m_Position.y - size.y, m_Position.z - size.z)); // 5 - MaxX MinY MinZ
    m_OriginVectors.push_back(glm::vec3(m_Position.x + size.x, m_Position.y + size.y, m_Position.z - size.z)); // 6 - MaxX MaxY MinZ
    m_OriginVectors.push_back(glm::vec3(m_Position.x - size.x, m_Position.y + size.y, m_Position.z - size.z)); // 7 - MinX MaxY MinZ

    m_Vertices = {
        m_OriginVectors[0].x, m_OriginVectors[0].y, m_OriginVectors[0].z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,
        m_OriginVectors[1].x, m_OriginVectors[1].y, m_OriginVectors[1].z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,
        m_OriginVectors[2].x, m_OriginVectors[2].y, m_OriginVectors[2].z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,
        m_OriginVectors[3].x, m_OriginVectors[3].y, m_OriginVectors[3].z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,
        m_OriginVectors[4].x, m_OriginVectors[4].y, m_OriginVectors[4].z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,
        m_OriginVectors[5].x, m_OriginVectors[5].y, m_OriginVectors[5].z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,
        m_OriginVectors[6].x, m_OriginVectors[6].y, m_OriginVectors[6].z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,
        m_OriginVectors[7].x, m_OriginVectors[7].y, m_OriginVectors[7].z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,
    };

    m_BoundMin.x = m_Position.x - size.x;
    m_BoundMax.x = m_Position.x + size.x;
    m_BoundMin.y = m_Position.y - size.y;
    m_BoundMax.y = m_Position.y + size.y;
    m_BoundMin.z = m_Position.z - size.z;
    m_BoundMax.z = m_Position.z + size.z;
}

void AABB::UpdatePosition(glm::vec3 position)
{
    m_Position = position;
}

void AABB::Update(glm::vec3 position, glm::vec3 rotation, glm::vec3 scaleObject)
{
    if (position != m_Position || rotation != m_Rotation || scaleObject != m_ScaleObject)
    {
        glm::mat4 transform = Math::CreateTransform(position, rotation, scaleObject);

        size_t vertexCount = m_Vertices.size() / m_VertexStride;

        m_BoundMin.x, m_BoundMin.y, m_BoundMin.z = std::numeric_limits<float>::max();
        m_BoundMax.x, m_BoundMax.y, m_BoundMax.z = std::numeric_limits<float>::min();

        for (size_t i = 0; i < vertexCount; i++)
        {
            glm::vec4 originVertex = glm::vec4(m_OriginVectors[i].x, m_OriginVectors[i].y, m_OriginVectors[i].z, 1.0f);

            glm::vec4 newVertex = transform * originVertex;

            // printf("AABB::Update newVertex [ %.2ff %.2ff %.2ff ]\n", newVertex.x, newVertex.y, newVertex.z);

            if (i == 0) {
                m_BoundMin = newVertex;
                m_BoundMax = newVertex;
            }

            if (newVertex.x < m_BoundMin.x) m_BoundMin.x = newVertex.x;
            if (newVertex.x > m_BoundMax.x) m_BoundMax.x = newVertex.x;
            if (newVertex.y < m_BoundMin.y) m_BoundMin.y = newVertex.y;
            if (newVertex.y > m_BoundMax.y) m_BoundMax.y = newVertex.y;
            if (newVertex.z < m_BoundMin.z) m_BoundMin.z = newVertex.z;
            if (newVertex.z > m_BoundMax.z) m_BoundMax.z = newVertex.z;
        }

        m_Vertices = {
            m_BoundMin.x, m_BoundMin.y, m_BoundMax.z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,    // 0 - MinX MinY MaxZ
            m_BoundMax.x, m_BoundMin.y, m_BoundMax.z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,    // 1 - MaxX MinY MaxZ
            m_BoundMax.x, m_BoundMax.y, m_BoundMax.z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,    // 2 - MaxX MaxY MaxZ
            m_BoundMin.x, m_BoundMax.y, m_BoundMax.z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,    // 3 - MinX MaxY MaxZ
            m_BoundMin.x, m_BoundMin.y, m_BoundMin.z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,    // 4 - MinX MinY MinZ
            m_BoundMax.x, m_BoundMin.y, m_BoundMin.z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,    // 5 - MaxX MinY MinZ
            m_BoundMax.x, m_BoundMax.y, m_BoundMin.z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,    // 6 - MaxX MaxY MinZ
            m_BoundMin.x, m_BoundMax.y, m_BoundMin.z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,    // 7 - MinX MaxY MinZ
        };

        m_Position = position;
        m_Rotation = rotation;
        m_ScaleObject = scaleObject;

        // printf("AABB::Update m_BoundMin [ %.2ff %.2ff %.2ff ]\n", m_BoundMin.x, m_BoundMin.y, m_BoundMin.z);
        // printf("AABB::Update m_BoundMax [ %.2ff %.2ff %.2ff ]\n", m_BoundMax.x, m_BoundMax.y, m_BoundMax.z);
        // printf("AABB::Update scaleObject [ %.2ff %.2ff %.2ff ]\n", scaleObject.x, scaleObject.y, scaleObject.z);
        // printf("AABB::Update m_ScaleAABB [ %.2ff %.2ff %.2ff ]\n", m_ScaleAABB.x, m_ScaleAABB.y, m_ScaleAABB.z);
    }
}

glm::vec3 AABB::GetMin() const
{
    return m_BoundMin;
}

glm::vec3 AABB::GetMax() const
{
    return m_BoundMax;
}

void AABB::Draw(Shader* shader, glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
    m_Indices = {
        0, 1,
        1, 2,
        2, 3,
        3, 0,

        4, 5,
        5, 6,
        6, 7,
        7, 4,

        0, 4,
        1, 5,
        2, 6,
        3, 7,
    };

    unsigned int m_LineVAO;
    unsigned int m_LineVBO;
    unsigned int m_LineEBO;

    glGenVertexArrays(1, &m_LineVAO);
    glGenBuffers(1, &m_LineVBO);
    glGenBuffers(1, &m_LineEBO);

    glBindVertexArray(m_LineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
    glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(float), m_Vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_LineEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(unsigned int), m_Indices.data(), GL_STATIC_DRAW);

    // Vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);

    // Vertex Color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    shader->Bind();
    glm::mat4 model = glm::mat4(1.0f);
    shader->setMat4("model", model);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);

    glBindVertexArray(m_LineVAO);
    glDrawElements(GL_LINES, (GLsizei)m_Indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glDeleteBuffers(1, &m_LineEBO);
    glDeleteBuffers(1, &m_LineVBO);
    glDeleteVertexArrays(1, &m_LineVAO);
}

bool AABB::Contains(glm::vec3 position, glm::vec3 scale)
{
	bool contains = !(
		m_Position.x + m_ScaleAABB.x < position.x || position.x + scale.x < m_Position.x ||
		m_Position.y + m_ScaleAABB.y < position.y || position.y + scale.y < m_Position.y ||
		m_Position.z + m_ScaleAABB.z < position.z || position.z + scale.z < m_Position.z);
	return contains;
}

/*
	* https://www.toptal.com/game/video-game-physics-part-ii-collision-detection-for-solid-objects
	*/
bool AABB::TestAABBOverlap(AABB * a, AABB * b)
{
	float d1x = b->m_Position.x - a->m_Position.x + a->m_ScaleAABB.x;
	float d1y = b->m_Position.y - a->m_Position.y + a->m_ScaleAABB.y;
	float d1z = b->m_Position.z - a->m_Position.z + a->m_ScaleAABB.z;
	float d2x = a->m_Position.x - b->m_Position.x + b->m_ScaleAABB.x;
	float d2y = a->m_Position.y - b->m_Position.y + b->m_ScaleAABB.y;
	float d2z = a->m_Position.z - b->m_Position.z + b->m_ScaleAABB.z;

	if (d1x > 0.0f || d1y > 0.0f || d1z > 0.0f)
		return false;

	if (d2x > 0.0f || d2y > 0.0f || d2z > 0.0f)
		return false;

	return true;
}

/*
 * Taken from JOML Intersectionf::intersectRayAab
 * https://github.com/JOML-CI/JOML/blob/master/src/org/joml/Intersectionf.java
 *
 */
bool AABB::IntersectRayAab(glm::vec3 origin, glm::vec3 dir, glm::vec3 min, glm::vec3 max, glm::vec2 result)
{
    float NaN = std::numeric_limits<float>::min();
    glm::vec3 invDir = glm::vec3(1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z);
    float tNear, tFar, tymin, tymax, tzmin, tzmax = NaN;

    if (invDir.x >= 0.0f) {
        tNear = (min.x - origin.x) * invDir.x;
        tFar = (max.x - origin.x) * invDir.x;
    }
    else {
        tNear = (max.x - origin.x) * invDir.x;
        tFar = (min.x - origin.x) * invDir.x;
    }
    if (invDir.y >= 0.0f) {
        tymin = (min.y - origin.y) * invDir.y;
        tymax = (max.y - origin.y) * invDir.y;
    }
    else {
        tymin = (max.y - origin.y) * invDir.y;
        tymax = (min.y - origin.y) * invDir.y;
    }
    if (tNear > tymax || tymin > tFar)
        return false;
    if (invDir.z >= 0.0f) {
        tzmin = (min.z - origin.z) * invDir.z;
        tzmax = (max.z - origin.z) * invDir.z;
    }
    else {
        tzmin = (max.z - origin.z) * invDir.z;
        tzmax = (min.z - origin.z) * invDir.z;
    }
    if (tNear > tzmax || tzmin > tFar)
        return false;
    tNear = tymin > tNear || tNear == NaN ? tymin : tNear;
    tFar = tymax < tFar || tFar == NaN ? tymax : tFar;
    tNear = tzmin > tNear ? tzmin : tNear;
    tFar = tzmax < tFar ? tzmax : tFar;
    if (tNear < tFar && tFar >= 0.0f) {
        result.x = tNear;
        result.y = tFar;
        return true;
    }
    return false;
}

AABB::~AABB()
{
}
