#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vector>
#include <utility>
#include <stdexcept>
#include "Util.hpp"
#include "AABB.hpp"
#include "Octree.hpp"
#include <iterator>
#include <unordered_map>

// copy constructor
Octree::Octree(const Octree& o) :
    boundary(o.boundary),
    points(o.points),
    kids(o.kids),
    haveKids(o.haveKids),
    depth(o.depth)
{
}

Octree::Octree(const AABB& boundary, const int& depth) :
    boundary(boundary),
    haveKids(false),
    depth(depth)
    {
        kids.reserve(8);
}

Octree::Octree(const v3& center, const float& halfDimension, const int& depth) :
    Octree(AABB(center,halfDimension),depth)
{}

Octree::Octree(const v3& center, const float& halfDimension) :
    Octree(center,halfDimension,1) {}

int Octree::size() const {
    int acc = points.size();
    for (const auto& kid: kids) {
        acc += kid.size();
    }
    return acc;
}

bool Octree::del(const v3& p) {

    // Ignore objects that do not belong in this quad tree
    if (!boundary.containsPoint(p)) {
        return false; // object cannot be added
    }

    //std::cout << "del - My center is " << printV(boundary.center) << " with range " << boundary.halfDimension << "\n";
    //std::cout << "and I'm looking to delete " << printV(p) << "\n";
    if (haveKids) {
        //std::cout << "Have kids depth " << depth << "\n";
        for (auto& kid: kids) {
            const bool removed = kid.del(p);
            if (removed) {
                return true;
            }
        }
        //std::cout << "Could not erase " << printV(p) << "\n";
    } else {
        //std::cout << "Don't have kids\n";
        //std::cout << "My map size is " << points.size() << "\n";
        //std::cout << "Look here for " << printV(p) << "\n";
        if (points.find(p) != points.end()) {
            //std::cout << "I do contain " << printV(p) << "\n";
            points.erase(p);
            return true;
        } else {
            //std::cout << "I don't contain " << printV(p) << "\n";
            return false;
        }
    }
    throw std::runtime_error("Error: could not delete ele from octree");
    return false;
}

bool Octree::insert(const v3&p, const Id& id) {
    return insert(std::make_pair(p,id));
}

bool Octree::insert(const v3Id& p) {
    // Ignore objects that do not belong in this quad tree
    if (!boundary.containsPoint(p.first)) {
        return false; // object cannot be added
    }

    if (haveKids) {
        for (auto& kid: kids) {
            const bool worked = kid.insert(p);
            if (worked) {
                return true;
            }
        }
    } else {
        if (points.size() < node_capacity) {
            //std::cout << "Trying to insert " << printV(p.first) << " with id " << p.second << "\n";
            const auto ret = points.insert(p);
            bool worked = ret.second;
            if (worked) {
                return true;
            } else {
                throw std::runtime_error("Error: insert into this octree level failed");
            }
        } else {
            // Otherwise, makeKids and then add the point to whichever node will accept it
            makeKids();
            return insert(p);
        }
    }

    throw std::runtime_error("Octree - Error: could not insert into tree?");
    // Otherwise, the point cannot be inserted for some unknown reason (this should never happen)
    return false;
}

vId Octree::queryRange(const v3& center, const float& halfDimension) const {
    return queryRange(AABB(center,halfDimension));
}

vId Octree::queryRange(const AABB& range) const {
    // Prepare an array of results
    vId idsInRange;
    //idsInRange.reserve(4);

    // Automatically abort if the range does not intersect this quad
    if (!boundary.intersectsAABB(range)) {
        return idsInRange; // empty list
    }

    if (haveKids) {
        for (const auto& kid: kids) {
            concat(idsInRange, kid.queryRange(range));
        }
        // return all kids ids that are valid
    } else {
        // Check objects at this quad level
        for (const auto& p: points) {
            if (range.containsPoint(p.first)) {
                idsInRange.push_back(p.second);
            }
        }
    }

    return idsInRange;
}

void Octree::makeKids() {

    if (haveKids) {
        throw std::runtime_error("Error: shouldn't be calling makeKids on something that has already done so");
    }

    haveKids = true;
    auto c = boundary.center;
    auto h = boundary.halfDimension / 2.0f;

    int i = 0;
    kids.push_back( Octree( v3(c.x + h, c.y + h, c.z + h),h,depth+1 ));
    kids.push_back( Octree( v3(c.x - h, c.y - h, c.z - h),h,depth+1 ));
    kids.push_back( Octree( v3(c.x - h, c.y + h, c.z + h),h,depth+1 ));
    kids.push_back( Octree( v3(c.x + h, c.y - h, c.z + h),h,depth+1 ));
    kids.push_back( Octree( v3(c.x + h, c.y + h, c.z - h),h,depth+1 ));
    kids.push_back( Octree( v3(c.x - h, c.y - h, c.z + h),h,depth+1 ));
    kids.push_back( Octree( v3(c.x - h, c.y + h, c.z - h),h,depth+1 ));
    kids.push_back( Octree( v3(c.x + h, c.y - h, c.z - h),h,depth+1 ));
    
    // remove all objects at this level, insert into kids...
    for (const auto& p: points) {
        const bool worked = insert(p);
        if (!worked) {
            throw std::runtime_error("Error: when making kids, could not move elements from parent to child nodes");
        }
    }
    points.clear();
}

