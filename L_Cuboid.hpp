#ifndef MY_L_CUBOID_HPP
#define MY_L_CUBOID_HPP

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <string>
#include <iosfwd>
#include <sstream>
#include <math.h>
#include <utility>

#include "Util.hpp"
#include "MTV.hpp"

typedef std::pair<float, float> Projection;

class L_Cuboid {
    private:
        const vv3* originalVertices_; // don't change
        vv3 static getAxes(const vv3& edges1, const vv3& edges2);
        vv3 calcVertices(const vv3* vertices_ptr_in, const v3& pos, const fq& ori, const v3& scale);
        vv3 static calcEdges(const vv3& v);
        float static overlapAmount(const Projection& p1, const Projection& p2);
        std::pair<float, float> static project(const v3& axis_in, const vv3& verts);
        float calcFurthestVertex(const vv3& vertices);

    public:
        MTV static colliding(const L_Cuboid& s1, const L_Cuboid& s2);
        void recalc(const v3& pos, const fq& ori);

        const v3 scale;
        L_Cuboid(const vv3* face_verts_ptr, const v3 scale, v3 startPos);
        vv3 verts24; // 24 non-unique vertices, 4 per face, 6 faces
        vv3 vertices; // 8 vertices unique
        //vv3 edges; // 24 edges, between vertices
        vv3 uniqEdges; // 3>= edges unique
        float furthestVertex;
        friend std::ostream& operator<<(std::ostream&, const L_Cuboid&);
};

#endif
