#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

typedef glm::dvec3 v3;
typedef glm::dvec4 v4;
typedef glm::dquat dq;
typedef glm::dmat4 m4;
static const v3 forward(0.0,0.0,1.0);
static const v3 backward(0.0,0.0,-1.0);
static const v3 left(-1.0,0.0,0.0);
static const v3 right(1.0,0.0,0.0);
static const v3 up(0.0,1.0,0.0);
static const v3 down(0.0,1.0,0.0);

dq orient;
m4 trans;

void move(const v3& v) {
    trans = glm::translate(trans, v * orient);
}

void q() {
    orient = dq(glm::vec3(0.0, -glm::radians(1.0), 0.0)) * orient;
}

void rotate(glm::dvec2 offset) {
    orient = dq(0.05 * v3(-glm::radians(offset.y), glm::radians(offset.x), 0.0)) * orient;
}

m4 update() {
    const v3 pos = trans * v4(0.0, 0.0, 0.0, 1.0);
    const v3 facing = forward * orient;
    const v3 up_relative = up * orient;
    return glm::lookAt(pos, pos + facing, up_relative);
}

int main() {
}
