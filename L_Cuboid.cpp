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
#include "L_Cuboid.hpp"

/* Now this L_Cuboid effectively only caches the state of the cuboid
 * In world space. You must manually call recalc when it has moved
 * To recalc the position of the vertices.
 * This is effectively a cache, the flag must be set elsewhere though
 */

vv3 L_Cuboid::getAxes(const vv3& edges1, const vv3& edges2) {
    // "The axes you must test are the normals of each shape’s edges."
    vv3 axes;
    vv3 normals;
    for (const auto& e1: edges1) {
        for (const auto& e2: edges2) {
            const auto t = glm::normalize(glm::cross(e1,e2));
            if (!std::isnan(t.x) && !std::isnan(t.y) && !std::isnan(t.z)) {
                normals.push_back(t);
            }
        }
    }
    concat(axes, edges1);
    concat(axes, edges2);
    concat(axes, normals);
    return axes;
}

bool L_Cuboid::colliding(const L_Cuboid& s1, const L_Cuboid& s2) {
    vv3 allAxes = getAxes(s1.uniqEdges, s2.uniqEdges);
    assert(allAxes.size() <= 15 && "Should get <= 15 axes");
    auto overlap = [&] (const Projection& p1, const Projection& p2) -> bool {
        return (p1.second >= p2.first) && (p1.first <= p2.second);
    };
    for (const auto& axis: allAxes) {
        Projection projection1 = project(axis, s1.vertices);
        Projection projection2 = project(axis, s2.vertices);
        if (!overlap(projection1,projection2)) {
            return false;
        }
    }
    return true;
}

Projection L_Cuboid::project(const v3& axis_in, const vv3& verts) {
    const v3 axis = glm::normalize(axis_in);
    float min = glm::dot(axis,verts[0]);
    float max = min;
    for (int i = 1; i < verts.size(); i++) {
        // NOTE: the axis must be normalized to get accurate projections
        float p = glm::dot(axis,verts[i]);
        if (p < min) {
            min = p;
        }
        if (p > max) {
            max = p;
        }
    }
    Projection proj = std::make_pair(min, max);
    return proj;
}


// builds a cuboid that matches the graphical coordinates
L_Cuboid::L_Cuboid(const fv* points_in, v3 topCenter) :
    originalTopCenter(topCenter) {
    const fv& points = *points_in;
    // first calc the faces
    const int size = points.size(); // 3d
    // 108 points -> faces
    for (int i=0; i<size; i+=18) {
        vv3 square;
        square.push_back(v3(points[i+0], points[i+1], points[i+2]));
        square.push_back(v3(points[i+3], points[i+4], points[i+5]));
        square.push_back(v3(points[i+6], points[i+7], points[i+8]));
        square.push_back(v3(points[i+9], points[i+10], points[i+11]));
        square.push_back(v3(points[i+12], points[i+13], points[i+14]));
        square.push_back(v3(points[i+15], points[i+16], points[i+17]));
        square = unique(square);
        concat(faces, square);
    }

    // all the unique points in the faces are the verts, size 8
    vertices = unique(faces);

    recalc();

    for (auto& v: vertices) {
        furthestVertex = std::max(furthestVertex, glm::length(v));
    }
}

vv3 L_Cuboid::calcVertices(const vv3& vertices, const v3& pos, const fq& ori) {
    const int verticesSize = vertices.size();
    vv3 world_vertices(verticesSize);
    for (int i=0; i<verticesSize; ++i) {
        v3 vertex = vertices[i];
        //vertex *= scale; // order of these lines matters
        vertex = ori * vertex;
        vertex += pos;
        world_vertices[i] = vertex;
    }
    return world_vertices;
}

vv3 L_Cuboid::calcEdges(const vv3& v) {
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

// should be done after cuboid is moved/changed
void L_Cuboid::recalc(const v3& pos, const fq& ori) {
    const vv3 verts24 = calcVertices(vertices,pos,ori);
    vertices = unique(verts24);
    edges = calcEdges(verts24);
    uniqEdges = unique(edges,true);
    topCenter = originalTopCenter * ori;
}

std::ostream& operator<<(std::ostream& stream, const L_Cuboid& c) {
    return stream << "Logical cuboid here";
}
