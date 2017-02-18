#ifndef OCTTREE_H
#define OCTTREE_H

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <vector>
#include <utility>
#include <map>

#include "Util.hpp"
#include "AABB.hpp"

struct cmp_v3 {
    bool operator()(const v3& a, const v3& b) const {
        return (a.x < b.x) || (a.z < b.z) || (a.y < b.y);
    }
};


class Octree {
    public:
        static const int node_capacity = 64;
        AABB boundary;
        std::map<v3, Id, cmp_v3> points;
        std::vector<Octree> kids;
        bool haveKids;
        // up-left, up-right, up-back-left, up-back-right
        // down-left, down-right, down-back-left, down-back-right
        Octree& operator=(const Octree&);
        int size() const;
        Octree(const Octree& o);
        Octree(const AABB& boundary);
        Octree(const v3& center, const float& halfDimension);
        bool insert(const v3Id&);
        bool insert(const v3&p, const Id& id);
        bool del(const v3&);
        vId queryRange(const v3& center, const float& halfDimension) const;
        vId queryRange(const AABB& range) const;
        void makeKids();
};

#endif
