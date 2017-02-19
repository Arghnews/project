#ifndef OCTTREE_H
#define OCTTREE_H

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vector>
#include <utility>
#include <unordered_map>

#include "Util.hpp"
#include "AABB.hpp"

class Octree {
    private:
        Octree(const AABB& boundary, const int& depth);
        Octree(const v3& center, const float& halfDimension, const int& depth);
    public:
        static const int node_capacity = 32;
        Octree(const v3& center, const float& halfDimension);
        AABB boundary;
        std::unordered_map<v3, Id> points;
        std::vector<Octree> kids;
        bool haveKids;
        int depth;
        // up-left, up-right, up-back-left, up-back-right
        // down-left, down-right, down-back-left, down-back-right
        int size() const;
        Octree(const Octree& o);
        bool insert(const v3Id&);
        bool insert(const v3&p, const Id& id);
        bool del(const v3&);
        vId queryRange(const v3& center, const float& halfDimension) const;
        vId queryRange(const AABB& range) const;
        void makeKids();
};

#endif
