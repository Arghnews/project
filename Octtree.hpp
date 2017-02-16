#ifndef OCTTREE_H
#define OCTTREE_H

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <vector>
#include <utility>

#include "Util.hpp"
#include "AABB.hpp"

class Octtree {
    public:
        const int node_capacity;
        AABB boundary;
        int size_;
        vv3Id points;
        std::vector<Octtree> kids;
        bool haveSubdivided;
        // up-left, up-right, up-back-left, up-back-right
        // down-left, down-right, down-back-left, down-back-right
        int size() const;
        Octtree(const Octtree& o);
        Octtree(const AABB& boundary, const int& node_capacity);
        Octtree(const v3& center, const float& halfDimension, const int& node_capacity);
        bool insert(const v3&, const Id&);
        bool insert(const v3Id&);
        bool del(const v3&, const Id&);
        bool del(const v3Id&);
        vv3Id queryRange(const v3& center, const float& halfDimension);
        vv3Id queryRange(const AABB& range);
        void subdivide();
};

#endif
