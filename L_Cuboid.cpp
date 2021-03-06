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
#include "MTV.hpp"

/* Now this L_Cuboid effectively only caches the state of the cuboid
 * In world space. You must manually call recalc when it has moved
 * To recalc the position of the vertices.
 * This is effectively a cache, the flag must be set elsewhere though
 */

// returns normalized axises between edges
vv3 L_Cuboid::getAxes(const vv3& edges1, const vv3& edges2) {
    // "The axes you must test are the normals of each shape’s edges."
    assert(edges1.size() <= 9);
    assert(edges2.size() <= 9);
    vv3 axes;
    axes.reserve(9);
    vv3 normals;
    for (const auto& e1: edges1) {
        for (const auto& e2: edges2) {
            const auto axis = glm::normalize(glm::cross(e1,e2));
            if (!std::isnan(axis.x) && !std::isnan(axis.y) && !std::isnan(axis.z)) {
                normals.push_back(axis);
            }
        }
    }
    concat(axes, edges1);
    concat(axes, edges2);
    concat(axes, normals);
    return axes;
}

MTV L_Cuboid::colliding(const L_Cuboid& s1, const L_Cuboid& s2) {
    float the_overlap = 1e8f;
    v3 smallestAxis;

    vv3 allAxes(unique(getAxes(s1.uniqEdges, s2.uniqEdges),true));

    auto overlap = [&] (const Projection& p1, const Projection& p2) -> bool {
        return (p1.second >= p2.first) && (p1.first <= p2.second);
    };

    for (const auto& axis: allAxes) {
        Projection p1(project(axis, s1.vertices));
        Projection p2(project(axis, s2.vertices));
        if (!overlap(p1,p2)) {
            MTV mtv;
            mtv.colliding = false;
            return mtv;
        } else {
            float o = overlapAmount(p1,p2);
            if (o < the_overlap) {
                the_overlap = o;
                smallestAxis = axis;
            }
        }
    }
    MTV mtv;
    mtv.colliding = true;
    mtv.overlap = the_overlap;
    mtv.axis = smallestAxis;
    return mtv;
}

float L_Cuboid::overlapAmount(const Projection& p1, const Projection& p2) {
    float ret;
    if (p1.first <= p2.first && p1.second <= p2.second) {
        ret = p1.second - p2.first;
    } else if (p2.first <= p1.first && p2.second <= p1.second) {
        ret = p2.second - p1.first;
    } else if (p1.first >= p2.first && p1.second <= p2.second) {
        ret = p1.second - p1.first;
    } else if (p1.first <= p2.first && p1.second >= p2.second) {
        ret = p2.second - p2.first;   
    } else {
        assert(false && "Holy poopdick you should never get here in overlaps");
    }
    return ret;
}

// NOTE: the axis must be normalized to get accurate projections
// allAxes returns normalized axes
Projection L_Cuboid::project(const v3& axis, const vv3& verts) {
    assert(verts.size() == 8);
    float min = glm::dot(axis,verts[0]);
    float max = min;
    for (int i = 1; i < verts.size(); ++i) {
        float p = glm::dot(axis,verts[i]);
        min = std::min(p,min);
        max = std::max(p,max);
        /*
        if (p < min) {
            min = p;
        }
        if (p > max) {
            max = p;
        }
        */
    }
    return std::make_pair(min, max);
}

// builds a cuboid that matches the graphical coordinates
L_Cuboid::L_Cuboid(const vv3* face_verts_ptr, const v3 scale, v3 startPos) :
    originalVertices_(face_verts_ptr),
    scale(scale),
    furthestVertex(0.0f) {
    /*
    const fv& points = *points_in;
    // first calc the faces
    const int size = points.size(); // 3d
    // 108 points -> faces
    vv3 faces; // 24 vertices
    assert(size == 108);
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
    assert(faces.size() == 24);

    // all the unique points in the faces are the verts, size 8
    originalVertices_ = faces;
    */

    recalc(startPos,fq());

    // position at origin
    const vv3 verts = calcVertices(originalVertices_,zeroV,fq(),scale);
    for (const auto& v: verts) {
        furthestVertex = std::max(furthestVertex, glm::length(v));
    }
    furthestVertex *= 2.0f;
}

vv3 L_Cuboid::calcVertices(const vv3* vertices_ptr_in, const v3& pos, const fq& ori, const v3& scale) {
    const vv3& vertices = *vertices_ptr_in;
    const int verticesSize = vertices.size();
    vv3 world_vertices(verticesSize);
    for (int i=0; i<verticesSize; ++i) {
        v3 vertex = vertices[i];
        vertex = ori * vertex;
        vertex *= scale; // order of these lines matters
        vertex += pos;
        world_vertices[i] = vertex;
    }
    return world_vertices;
}

vv3 L_Cuboid::calcEdges(const vv3& v) {
    // calcs edges for cuboid
    const int size = v.size();
    vv3 e(size);
    for (int i=0; i<size; i+=4) {
        e[i+0] = v[i+0] - v[i+1];
        e[i+1] = v[i+1] - v[i+2];
        e[i+2] = v[i+2] - v[i+3];
        e[i+3] = v[i+3] - v[i+0];
    }
    return e;
}

// should be done after cuboid is moved/changed
void L_Cuboid::recalc(const v3& pos, const fq& ori) {
    verts24 = calcVertices(originalVertices_,pos,ori,scale);
    assert(verts24.size() == 24);
    //edges = calcEdges(verts24);
    vertices = unique(verts24);
    uniqEdges = unique(calcEdges(verts24),true);
}

std::ostream& operator<<(std::ostream& stream, const L_Cuboid& c) {
    return stream << "Logical cuboid here";
}
