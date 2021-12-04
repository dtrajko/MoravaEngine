/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include <glm/glm.hpp>


namespace H2M
{

	struct AABB_H2M
	{

		glm::vec3 Min, Max;

        AABB_H2M()
			: Min(0.0f), Max(0.0f) {}

        AABB_H2M(const glm::vec3& min, const glm::vec3& max)
			: Min(min), Max(max) {}

		bool Intersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float& t) const
		{
            glm::vec3 dirfrac;
            // r.dir is unit direction vector of ray
            dirfrac.x = 1.0f / rayDirection.x;
            dirfrac.y = 1.0f / rayDirection.y;
            dirfrac.z = 1.0f / rayDirection.z;
            // lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
            // r.org is origin of ray
            const glm::vec3& lb = Min;
            const glm::vec3& rt = Max;
            float t1 = (lb.x - rayOrigin.x) * dirfrac.x;
            float t2 = (rt.x - rayOrigin.x) * dirfrac.x;
            float t3 = (lb.y - rayOrigin.y) * dirfrac.y;
            float t4 = (rt.y - rayOrigin.y) * dirfrac.y;
            float t5 = (lb.z - rayOrigin.z) * dirfrac.z;
            float t6 = (rt.z - rayOrigin.z) * dirfrac.z;

            float tmin = glm::max(glm::max(glm::min(t1, t2), glm::min(t3, t4)), glm::min(t5, t6));
            float tmax = glm::min(glm::min(glm::max(t1, t2), glm::max(t3, t4)), glm::max(t5, t6));

            // if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
            if (tmax < 0)
            {
                t = tmax;
                return false;
            }

            // if tmin > tmax, ray doesn't intersect AABB
            if (tmin > tmax)
            {
                t = tmax;
                return false;
            }

            t = tmin;
            return true;
		}

	};

}
