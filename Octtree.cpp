#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <vector>
#include <utility>
#include <stdexcept>
#include "Util.hpp"
#include "AABB.hpp"
#include "Octtree.hpp"
#include <iterator>

Octtree::Octtree(const Octtree& o) :
    node_capacity(o.node_capacity),
    boundary(o.boundary),
    points(o.points),
    kids(o.kids),
    size_(o.size_),
    haveSubdivided(o.haveSubdivided)
{
}

Octtree::Octtree(const AABB& boundary, const int& node_capacity) :
    boundary(boundary),
    haveSubdivided(false),
    size_(0),
    node_capacity(node_capacity)
{
}

Octtree::Octtree(const v3& center, const float& halfDimension, const int& node_capacity) :
    Octtree(AABB(center,halfDimension),node_capacity)
{
}

int Octtree::size() const {
    int acc = size_;
    for (const auto& kid: kids) {
        acc += kid.size();
    }
    return acc;
}

bool Octtree::del(const v3& v, const Id& id) {
    return del(std::make_pair(v,id));
}

bool Octtree::del(const v3Id& p) {
    for (auto i = points.begin(); i != points.end(); ++i) {
        const v3& point = (*i).first;
        const Id& id = (*i).second;
        const bool samePlace = areSame(point,p.first);
        const bool samePointer = (id == p.second);
        if (samePointer && samePointer) {
            points.erase(i);
            --size_;
            return true;
        }
    }

    for (auto& kid: kids) {
        const bool removed = kid.del(p);
        if (removed) {
            return true;
        }
    }
    //std::cout << "Could not find " << printV(p.first) << "," << (p.second) << "\n";
    return false;
}

bool Octtree::insert(const v3& v, const Id& id) {
    insert(std::make_pair(v,id));
}

bool Octtree::insert(const v3Id& p) {
    // Ignore objects that do not belong in this quad tree
    if (!boundary.containsPoint(p.first)) {
        return false; // object cannot be added
    }

    // If there is space in this quad tree, add the object here
    if (points.size() < node_capacity) {
        points.push_back(p);
        ++size_;
        return true;
    }

    // Otherwise, subdivide and then add the point to whichever node will accept it
    if (!haveSubdivided) {
        subdivide();
    }

    for (auto& kid: kids) {
        if (kid.insert(p)) {
            return true;
        }
    }
    throw std::runtime_error("Error: could not insert into tree?");
    // Otherwise, the point cannot be inserted for some unknown reason (this should never happen)
    return false;
}

/*
vv3Id Octtree::queryPoint(const v3& point) {
    
}*/

vv3Id Octtree::queryRange(const v3& center, const float& halfDimension) {
    return queryRange(AABB(center,halfDimension));
}

vv3Id Octtree::queryRange(const AABB& range) {
    // Prepare an array of results
    vv3Id pointsInRange;

    // Automatically abort if the range does not intersect this quad
    if (!boundary.intersectsAABB(range)) {
        return pointsInRange; // empty list
    }

    // Check objects at this quad level
    for (int p = 0; p < points.size(); ++p) {
        if (range.containsPoint(points[p].first)) {
            pointsInRange.push_back(points[p]);
        }
    }

    // Terminate here, if there are no children
    if (!haveSubdivided) {
        return pointsInRange;
    }

    for (auto& kid: kids) {
        concat(pointsInRange, kid.queryRange(range));
    }
    // Otherwise, add the points from the children

    return pointsInRange;
}

void Octtree::subdivide() {

    if (haveSubdivided) {
        throw std::runtime_error("Error: shouldn't be calling subdivide on something that has already done so");
    }
    haveSubdivided = true;
    auto& c = boundary.center;
    auto h = boundary.halfDimension / 2.0f;

    kids.push_back(Octtree( v3(c.x + h, c.y + h, c.z + h),h,node_capacity ));
    kids.push_back(Octtree( v3(c.x - h, c.y - h, c.z - h),h,node_capacity ));
    kids.push_back(Octtree( v3(c.x - h, c.y + h, c.z + h),h,node_capacity ));
    kids.push_back(Octtree( v3(c.x + h, c.y - h, c.z + h),h,node_capacity ));
    kids.push_back(Octtree( v3(c.x + h, c.y + h, c.z - h),h,node_capacity ));
    kids.push_back(Octtree( v3(c.x - h, c.y - h, c.z + h),h,node_capacity ));
    kids.push_back(Octtree( v3(c.x - h, c.y + h, c.z - h),h,node_capacity ));
    kids.push_back(Octtree( v3(c.x + h, c.y - h, c.z - h),h,node_capacity ));
}

