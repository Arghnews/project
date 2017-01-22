#ifndef MY_AABB
#define MY_AABB

#include "Util.hpp"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

struct AABB {
    AABB();
    AABB(v3, float);
    v3 center;
    float halfDimension;
    bool containsPoint(v3 point);
    bool intersectsAABB(AABB other);
};
#endif
