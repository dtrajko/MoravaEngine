#pragma once

#include <Jolt/Jolt.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


// Between glm (the engine) and Jolt's math types. Positions: Jolt's RVec3 is its Vec3 as long as it is built without
// DOUBLE_PRECISION
inline JPH::Vec3 ToJolt(const glm::vec3& v) { return JPH::Vec3(v.x, v.y, v.z); }
inline JPH::Quat ToJolt(const glm::quat& q) { return JPH::Quat(q.x, q.y, q.z, q.w); }

inline glm::vec3 ToGlm(JPH::Vec3Arg v) { return glm::vec3(v.GetX(), v.GetY(), v.GetZ()); }
inline glm::quat ToGlm(JPH::QuatArg q) { return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ()); }
