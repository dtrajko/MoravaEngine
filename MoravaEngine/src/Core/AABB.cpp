#include "Core/AABB.h"

#include "Core/Math.h"

#include <limits>


AABB::AABB() : AABB(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f))
{
}

AABB::AABB(glm::vec3 positionOrigin, glm::quat rotationOrigin, glm::vec3 scaleOrigin)
{
    m_IsColliding = false;

    m_BoundMin = glm::vec3(
        positionOrigin.x - m_UnitSize * scaleOrigin.x,
        positionOrigin.y - m_UnitSize * scaleOrigin.y,
        positionOrigin.z - m_UnitSize * scaleOrigin.z
    );

    m_BoundMax = glm::vec3(
        positionOrigin.x + m_UnitSize * scaleOrigin.x,
        positionOrigin.y + m_UnitSize * scaleOrigin.y,
        positionOrigin.z + m_UnitSize * scaleOrigin.z
    );

    //  printf("AABB::AABB positionOrigin [ %.2ff %.2ff %.2ff ] scaleOrigin [ %.2ff %.2ff %.2ff ]\n",
    //      positionOrigin.x, positionOrigin.y, positionOrigin.z, scaleOrigin.x, scaleOrigin.y, scaleOrigin.z);
    //  printf("AABB::AABB m_UnitSize [ %.2ff ]\n", m_UnitSize);
    //  printf("AABB::AABB m_BoundMin [ %.2ff %.2ff %.2ff ] m_BoundMax [ %.2ff %.2ff %.2ff ]\n",
    //      m_BoundMin.x, m_BoundMin.y, m_BoundMin.z, m_BoundMax.x, m_BoundMax.y, m_BoundMax.z);

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

    m_VerticesInitial = m_Vertices;

    m_Position = positionOrigin;
    m_Rotation = rotationOrigin;
    m_Scale    = scaleOrigin;

    // m_PositionOrigin = positionOrigin;
    // m_ScaleOrigin = scaleOrigin;

    // printf("AABB::AABB::TransformBounds\n");
    TransformBounds(glm::vec3(0.0f), glm::quat(glm::vec3(0.0f)), glm::vec3(1.0f));
}

void AABB::UpdatePosition(glm::vec3 positionOrigin)
{
    m_PositionOrigin = positionOrigin;
}

void AABB::Update(glm::vec3 positionObject, glm::quat rotationObject, glm::vec3 scaleObject)
{
    // printf("AABB::Update scaleObject [ %.2ff %.2ff %.2ff ]\n", scaleObject.x, scaleObject.y, scaleObject.z);

    if (positionObject != m_Position || rotationObject != m_Rotation || scaleObject != m_Scale)
    {
        // printf("AABB::Update::TransformBounds\n");
        TransformBounds(positionObject, rotationObject, scaleObject);
    }
}

void AABB::TransformBounds(glm::vec3 positionObject, glm::quat rotationObject, glm::vec3 scaleObject)
{
    glm::vec3 transformPosition = m_PositionOrigin + positionObject;
    glm::vec3 transformScale = m_ScaleOrigin * scaleObject;
    glm::mat4 transform = Math::CreateTransform(transformPosition, rotationObject, transformScale);

    //  printf("AABB::TransformBounds m_PositionOrigin [ %.2ff %.2ff %.2ff ] positionObject [ %.2ff %.2ff %.2ff ]\n",
    //      m_PositionOrigin.x, m_PositionOrigin.y, m_PositionOrigin.z, positionObject.x, positionObject.y, positionObject.z);
    //  
    //  printf("AABB::TransformBounds m_ScaleOrigin [ %.2ff %.2ff %.2ff ] scaleObject [ %.2ff %.2ff %.2ff ]\n",
    //      m_ScaleOrigin.x, m_ScaleOrigin.y, m_ScaleOrigin.z, scaleObject.x, scaleObject.y, scaleObject.z);
    //  
    //  printf("AABB::TransformBounds transformPosition [ %.2ff %.2ff %.2ff ] transformPosition [ %.2ff %.2ff %.2ff ]\n",
    //      transformPosition.x, transformPosition.y, transformPosition.z, transformScale.x, transformScale.y, transformScale.z);

    size_t vertexCount = m_Vertices.size() / m_VertexStride;

    m_BoundMin.x, m_BoundMin.y, m_BoundMin.z = std::numeric_limits<float>::max();
    m_BoundMax.x, m_BoundMax.y, m_BoundMax.z = std::numeric_limits<float>::min();

    for (size_t i = 0; i < vertexCount; i++)
    {
        glm::vec4 originVertex = glm::vec4(
            m_VerticesInitial[i * m_VertexStride + 0],
            m_VerticesInitial[i * m_VertexStride + 1],
            m_VerticesInitial[i * m_VertexStride + 2], 1.0f);

        glm::vec4 newVertex = transform * originVertex;

        // printf("AABB::TransformBounds [%zu] originVertex [ %.2ff %.2ff %.2ff ]\n", i, originVertex.x, originVertex.y, originVertex.z);
        // printf("AABB::TransformBounds [%zu] newVertex [ %.2ff %.2ff %.2ff ]\n", i, newVertex.x, newVertex.y, newVertex.z);

        if (i == 0) {
            m_BoundMin = glm::vec3(
                newVertex.x - m_Offset,
                newVertex.y - m_Offset,
                newVertex.z - m_Offset);
            m_BoundMax = glm::vec3(
                newVertex.x + m_Offset,
                newVertex.y + m_Offset,
                newVertex.z + m_Offset);
        }

        if (newVertex.x < m_BoundMin.x) m_BoundMin.x = newVertex.x - m_Offset;
        if (newVertex.x > m_BoundMax.x) m_BoundMax.x = newVertex.x + m_Offset;
        if (newVertex.y < m_BoundMin.y) m_BoundMin.y = newVertex.y - m_Offset;
        if (newVertex.y > m_BoundMax.y) m_BoundMax.y = newVertex.y + m_Offset;
        if (newVertex.z < m_BoundMin.z) m_BoundMin.z = newVertex.z - m_Offset;
        if (newVertex.z > m_BoundMax.z) m_BoundMax.z = newVertex.z + m_Offset;
    }

    // printf("AABB::TransformBounds m_BoundMin [ %.2ff %.2ff %.2ff ]\n", m_BoundMin.x, m_BoundMin.y, m_BoundMin.z);
    // printf("AABB::TransformBounds m_BoundMax [ %.2ff %.2ff %.2ff ]\n", m_BoundMax.x, m_BoundMax.y, m_BoundMax.z);

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

    m_Position = positionObject;
    m_Rotation = rotationObject;
    m_Scale = scaleObject;

    // printf("AABB::TransformBounds m_Scale [ %.2ff %.2ff %.2ff ]\n", m_Scale.x, m_Scale.y, m_Scale.z);
}

glm::vec3 AABB::GetMin() const
{
    return m_BoundMin;
}

glm::vec3 AABB::GetMax() const
{
    return m_BoundMax;
}

void AABB::Draw()
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
		m_Position.x + (m_ScaleOrigin.x * m_Scale.x) < position.x || position.x + scale.x < m_Position.x ||
		m_Position.y + (m_ScaleOrigin.y * m_Scale.y) < position.y || position.y + scale.y < m_Position.y ||
		m_Position.z + (m_ScaleOrigin.z * m_Scale.z) < position.z || position.z + scale.z < m_Position.z);
	return contains;
}

/*
 * https://www.toptal.com/game/video-game-physics-part-ii-collision-detection-for-solid-objects
 */
bool AABB::TestAABBOverlap(AABB * a, AABB * b)
{
	float d1x = b->m_Position.x - a->m_Position.x + (a->m_ScaleOrigin.x * a->m_Scale.x);
    float d1y = b->m_Position.y - a->m_Position.y + (a->m_ScaleOrigin.y * a->m_Scale.y);
    float d1z = b->m_Position.z - a->m_Position.z + (a->m_ScaleOrigin.z * a->m_Scale.z);
    float d2x = a->m_Position.x - b->m_Position.x + (b->m_ScaleOrigin.x * b->m_Scale.x);
    float d2y = a->m_Position.y - b->m_Position.y + (b->m_ScaleOrigin.y * b->m_Scale.y);
    float d2z = a->m_Position.z - b->m_Position.z + (b->m_ScaleOrigin.z * b->m_Scale.z);

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
