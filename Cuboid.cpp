#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GL/glew.h> 
#include <string>
#include <iostream>
#include <sstream>
#include <math.h>
#include <utility>

#include "Util.hpp"
#include "Cuboid.hpp"

/* Changed cuboid to be more minimalist
 * Now doesn't store any non-local data, ie pos/orient
 * Must be given world data when want vertices on the fly
 */

Cuboid::Cuboid(fv points, v3 topCenter) :
    points_(points),
    topCenter(topCenter)
{
    const int size = points_.size(); // 3d
    for (int i=0; i<size; i+=18) {
        vv3 square;
        square.push_back(v3(points_[i+0], points_[i+1], points_[i+2]));
        square.push_back(v3(points_[i+3], points_[i+4], points_[i+5]));
        square.push_back(v3(points_[i+6], points_[i+7], points_[i+8]));
        square.push_back(v3(points_[i+9], points_[i+10], points_[i+11]));
        square.push_back(v3(points_[i+12], points_[i+13], points_[i+14]));
        square.push_back(v3(points_[i+15], points_[i+16], points_[i+17]));
        square = unique(square);
        concat(actualPoints_, square);
    }
    recalc();
    half_xyz_ = v3();
    // assumes centre 0,0,0 of shape
    for (const auto& v: vertices_) {
        half_xyz_ = glm::max(half_xyz_,glm::abs(v));
    }
    half_xyz_ /= 2.0f;

    furthestVertex_ = calcFurthestVertex(vertices_);
}

const fv* Cuboid::points() {
    return &points_;
}

vv3 Cuboid::getVertices(const v3& pos, const fq& ori) {
    const int verticesSize = actualPoints_.size();
    vv3 vertices(verticesSize);
    for (int i=0; i<verticesSize; ++i) {
        v3 vertex = actualPoints_[i];
        //vertex *= scale; // order of these lines matters
        vertex = ori * vertex;
        vertex += pos;
        vertices[i] = vertex;
    }
    return vertices;
}

vv3* Cuboid::actualPoints() {
    return &actualPoints_;
}

vv3 Cuboid::getUniqueEdges() {
    return uniqEdges_;
}

vv3 Cuboid::calcEdges(const vv3& v) {
    assert(v.size() == 24 && "Should have 24 verts to calc edges");
    // calcs edges for cuboid
    vv3 e;
    const int size = v.size();
    for (int i=0; i<size; i+=4) {
        vv3 face(4);
        face[0] = v[i+0];
        face[1] = v[i+1];
        face[2] = v[i+2];
        face[3] = v[i+3];
        const int faceSize = face.size();
        for (int j=0; j<faceSize; ++j) {
            e.push_back((face[j] - face[(j+1)%faceSize]));
        }
    }
    return e;
}

v3 Cuboid::half_xyz() {
    return half_xyz_;
}

float Cuboid::furthestVertex() const {
    return furthestVertex_;
}

float Cuboid::calcFurthestVertex(const vv3& vertices) {
    float d = 0.0f;
    for (auto& v: vertices) {
        d = std::max(d, glm::length(v));
    }
    return d;
}

const vv3* Cuboid::uniqueVertices() {
    return &vertices_;
}

// should be done after cuboid is moved/changed
void Cuboid::recalc(const v3& pos, const fq& ori) {
    vv3 verts24 = getVertices(pos,ori);
    vertices_ = unique(verts24);
    vv3 edges24 = calcEdges(verts24);
    edges_ = edges24;
    uniqEdges_ = unique(edges_,true);
}

std::ostream& operator<<(std::ostream& stream, const Cuboid& c) {
    return stream << "Cuboid here";
}
