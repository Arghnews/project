#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "Util.hpp"
#include "Camera.hpp"

Camera::Camera() {
}

void Camera::move(const v3& v) { // change translation matrix by v
    trans = glm::translate(trans, v * orient);
}

void Camera::turn(const v3& v) { // turn by vec
    //orient = dq(glm::vec3(0.0, -glm::radians(1.0), 0.0)) * orient;
    orient = v * orient;
}

void Camera::rotate(const v2& offset) { // rotate by mouse input
    orient = fq(0.05f * v3(glm::radians(offset.y), glm::radians(offset.x), 0.0f)) * orient;
}

m4 Camera::update() { // view matrix
    const v3 pos = trans * v4(0.0f, 0.0f, 0.0f, 1.0f);
    const v3 facing = FORWARD * orient;
    const v3 up_relative = UP * orient;
    return glm::lookAt(pos, pos + facing, up_relative);
}

