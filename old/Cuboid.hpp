#ifndef MY_CUBOID_H
#define MY_CUBOID_H
#define GLM_FORCE_RADIANS
#include "Util.hpp"
#include <GL/glew.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <math.h>

class Cuboid {
    private:
        fv points_; // 108 floats
        vv3 actualPoints_; // 24 vertices
        vv3 vertices_; // 8 vertices unique
        vv3 edges_; // 24 edges, between vertices
        vv3 uniqEdges_; // 3 edges
        vv3 static calcEdges(const vv3& v);
        v3 half_xyz_;
        float furthestVertex_;
        float calcFurthestVertex(const vv3& vertices);

    public:
        Cuboid(fv points, v3 topCenter);

        v3 topCenter;
        void rotateRads(const v3& ypr);
        void rotateQuat(const fq& q);
        void translate(v3 by);
        void setOrient(const fq& orient);

        float furthestVertex() const;
        v3 half_xyz();
        vv3* actualPoints();
        const vv3* uniqueVertices(); // 8
        vv3 getUniqueEdges(); // sign insensitive unique edges
        void recalc(const v3& pos=v3(), const fq& ori=fq());
        vv3 getVertices(const v3& pos,const fq& ori);
        const fv* points();

        friend std::ostream& operator<<(std::ostream&, const Cuboid&);
};

#endif
