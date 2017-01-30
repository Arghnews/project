#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "Util.hpp"
#include "AABB.hpp"

AABB::AABB(v3 center, float halfDimension) :
    center(center),
    halfDimension(halfDimension)
{
}

bool AABB::containsPoint(v3 point) {
    return (point.x <= center.x+halfDimension &&
            point.x >= center.x-halfDimension &&
            point.y <= center.y+halfDimension &&
            point.y >= center.y-halfDimension &&
            point.z <= center.z+halfDimension &&
            point.z >= center.z-halfDimension);
}

bool AABB::intersectsAABB(AABB other) {
    return (other.center.x-other.halfDimension <= center.x+halfDimension &&
            other.center.x+other.halfDimension >= center.x-halfDimension &&
            other.center.y-other.halfDimension <= center.y+halfDimension &&
            other.center.y+other.halfDimension >= center.y-halfDimension &&
            other.center.z-other.halfDimension <= center.z+halfDimension &&
            other.center.z+other.halfDimension >= center.z-halfDimension);
}

