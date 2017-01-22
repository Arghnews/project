#include "Util.hpp"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GL/glew.h> 
#include <GL/glut.h> 
#include <string>
#include <iostream>
#include <sstream>
#include <math.h>
#include <utility>

#include "Cuboid.hpp"
#include "State.hpp"

// USING RADIANS

Cuboid::Cuboid(fv points, v3 topCenter, v3 scale, v3 translationMultiplier, v3 rotationMultiplier) :
    points_(points),
    scale_(scale),
    translationMultiplier(translationMultiplier),
    rotationMultiplier(rotationMultiplier),
    originalTopCenter(scale*topCenter)
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
    recalcEdges();
    half_xyz_ = v3();
    // assumes centre 0,0,0 of shape
    for (const auto& v: vertices_) {
        half_xyz_ = glm::max(half_xyz_,glm::abs(v));
    }
    half_xyz_ /= 2.0f;

    state_.orient = fq(); // identity
    state_.pos = zeroV;
    state_.topCenter = topCenter*scale_;
    state_.rotation = zeroV;
    lastState_ = state_;
    furthestVertex_ = calcFurthestVertex();
}

State Cuboid::translate(v3 by) {
    if (by == zeroV) {
        return State();
    }
    lastState_.pos = state_.pos;
    state_.pos += by;
    recalcEdges();

    State s;
    s.pos = by;
    return s;
}

State Cuboid::setOrient(const fq& orient) {
    lastState_.orient = state_.orient;
    lastState_.topCenter = state_.topCenter;
    lastState_.rotation = state_.rotation;
    
    state_.orient = orient;
    state_.topCenter = orient * originalTopCenter;
    recalcEdges();

    State s;
    s.orient = lastState_.orient;
    s.topCenter = state_.topCenter - lastState_.topCenter;
    s.rotation = state_.rotation - lastState_.rotation;
    return s;
}

State Cuboid::rotateQuat(const fq& q) {
    if (q == fq()) {
        // if identity ie. no rotation
        return State();
    }
    const fq quat = q;

    lastState_.orient = state_.orient;
    lastState_.topCenter = state_.topCenter;
    lastState_.rotation = state_.rotation;

    state_.orient = quat * state_.orient;
    state_.topCenter = quat * state_.topCenter;
    recalcEdges();
    // the function that actually does the rotating
    State s;
    s.orient = quat;
    s.topCenter = state_.topCenter - lastState_.topCenter;
    s.rotation = state_.rotation - lastState_.rotation;
    return s;
}

State Cuboid::rotateRads(const v3& ypr) {
    // if no change just return
    return rotateQuat(fq(ypr));
}

const v3 Cuboid::scale() const {
    return scale_;
}

void Cuboid::state(State& s) {
    state_ = s;
}

State Cuboid::state() {
    return state_;
}

void Cuboid::lastState(State& s) {
    lastState_ = s;
}

State Cuboid::lastState() {
    return lastState_;
}

const fv* Cuboid::points() {
    return &points_;
}

vv3 Cuboid::getVertices() {
    const v3 centre = state_.pos;
    const fq orient = state_.orient;
    const int verticesSize = actualPoints_.size();
    vv3 vertices(verticesSize);
    for (int i=0; i<verticesSize; ++i) {
        v3 vertex = actualPoints_[i];
        vertex *= scale_; // must be before rotate
        vertex = orient * vertex;
        vertex += centre;
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

float Cuboid::furthestVertex() {
    return furthestVertex_;
}

float Cuboid::calcFurthestVertex() {
    float d = 0.0f;
    for (auto& v: vertices_) {
        d = std::max(d, glm::length(v));
    }
    return d;
}

const vv3* Cuboid::uniqueVertices() {
    return &vertices_;
}

void Cuboid::recalcEdges() {
    vv3 verts24 = getVertices();
    vertices_ = unique(verts24);
    vv3 edges24 = calcEdges(verts24);
    edges_ = edges24;
    uniqEdges_ = unique(edges_,true);
}

std::ostream& operator<<(std::ostream& stream, const Cuboid& c) {
    return stream << "Cuboid here";
}

