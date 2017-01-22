#ifndef MY_CUBOID
#define MY_CUBOID
#define GLM_FORCE_RADIANS
#include "Util.hpp"
#include <GL/glew.h> 
#include <GL/glut.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <math.h>

#include "State.hpp"

// USING RADIANS
class Cuboid {
    private:
        State state_;
        State lastState_;
        const v3 scale_;
        fv points_; // 108 floats
        vv3 actualPoints_; // 24 vertices
        vv3 vertices_; // 8 vertices unique
        vv3 edges_; // 24 edges, between vertices
        vv3 uniqEdges_; // 3 edges
        vv3 static calcEdges(const vv3& v);
        v3 half_xyz_;
        float furthestVertex_;
        float calcFurthestVertex();
        const v3 originalTopCenter;

    public:
        State rotateRads(const v3& ypr);
        State rotateQuat(const fq& q);
        State translate(v3 by);
        State setOrient(const fq& orient);

        void state(State& s);
        State state();
        void lastState(State& s);
        State lastState();
        float furthestVertex();
        Cuboid(fv points, v3 topCenter, v3 scale, v3 translationMultiplier, v3 rotationMultiplier);
        v3 translationMultiplier; // movement multiplier
        v3 rotationMultiplier; // rotation multiplier
        const v3 scale() const;
        v3 half_xyz();
        vv3* actualPoints();
        const vv3* uniqueVertices(); // 8
        vv3 getUniqueEdges(); // sign insensitive unique edges
        void recalcEdges();
        vv3 getVertices();
        const fv* points();

        bool static colliding(const Cuboid& c1, const Cuboid& c2);

        friend std::ostream& operator<<(std::ostream&, const Cuboid&);
};

#endif
