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
        static const int eight = 8;
        const int node_capacity = 1;
        AABB boundary;
        int size_;
        vv3S points;
        std::vector<Octtree> kids;
        bool haveSubdivided;
        // up-left, up-right, up-back-left, up-back-right
        // down-left, down-right, down-back-left, down-back-right
        int size();
        Octtree(const Octtree& o);
        Octtree(AABB boundary);
        Octtree(v3 center, float halfDimension);
        bool insert(v3, Shape*);
        bool insert(v3S);
        bool del(v3, Shape*);
        bool del(v3S p);
        vv3S queryRange(const v3 center, const float halfDimension);
        vv3S queryRange(AABB range);
        void subdivide();
};

#endif
