#ifndef MY_AABB
#define MY_AABB

#include "Util.hpp"
#include <glm/glm.hpp>

struct AABB {
    AABB();
    AABB(const v3& center, const float& halfDimension);
    AABB(const AABB& o);
    const v3 center;
    const float halfDimension;
    bool containsPoint(const v3& point) const;
    bool intersectsAABB(const AABB& other) const;
};
#endif
