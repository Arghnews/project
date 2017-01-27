#ifndef MY_L_CUBOID_HPP
#define MY_L_CUBOID_HPP

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <math.h>
#include <utility>

#include "Util.hpp"

typedef std::pair<float, float> Projection;

class L_Cuboid {
    private:
        const v3 originalTopCenter;
        vv3 static calcVertices(const vv3& vertices, const v3& pos, const fq& ori);
        vv3 static calcEdges(const vv3& v);
        float calcFurthestVertex(const vv3& vertices);
        void recalc(const v3& pos=v3(), const fq& ori=fq());
        vv3 static getAxes(const vv3& edges1, const vv3& edges2);
        std::pair<float, float> static project(const v3& axis_in, const vv3& verts);

    public:
        bool static colliding(const L_Cuboid& s1, const L_Cuboid& s2);

        L_Cuboid(const fv& points, v3 topCenter);
        vv3 faces; // 24 vertices
        vv3 vertices; // 8 vertices unique
        vv3 edges; // 24 edges, between vertices
        vv3 uniqEdges; // 3 edges
        v3 topCenter;
        float furthestVertex;
        friend std::ostream& operator<<(std::ostream&, const L_Cuboid&);
};

#endif
