#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Util.hpp"
#include "P_State.hpp"
#include "Force.hpp"

#include <iostream>
#include <sstream>

/* a simple class that encompasses the physical state of an object
 * Things like it's position, momentum, mass and orientation are here
*/

P_State::P_State(float m, float inertia, v3 pos) :
    mass(m),
    inverse_mass(1.0f/m),
    inertia(inertia),
    inverse_inertia(1.0f/inertia),
    position(pos) {
    }

m4 P_State::modelMatrix(const v3& scale) const {
    //glm::mat4 myModelMatrix = myTranslationMatrix * myRotationMatrix * myScaleMatrix;
    m4 model;
    m4 rotation(glm::toMat4(orient));
    m4 translation(glm::translate(m4(), position));
    model = translation * rotation * glm::scale(scale);
    return model;
}

m4 P_State::viewMatrix() const {
    const v3 facing = orient * FORWARD;
    const v3 up_relative = orient * UP;
    const v3 behindMe = orient * v3(0.0f,2.0f,3.0f);
    return glm::lookAt(position+behindMe, position + facing, up_relative);
}

void P_State::recalc() {

    velocity = momentum * inverse_mass;

    ang_velocity = ang_momentum * 
        inverse_inertia;

    orient = glm::normalize(orient);

    spin = 0.5f * fq(ang_velocity) * orient;
}

void P_State::apply_force(const Force& f) {
    v3 force = f.force;
    if (f.relative) {
        force = orient * force;
    }
    if (force != zeroV) {
        forces.push_back(force);
    }
    //const v3 torque = glm::cross(force, point - position);
    if (f.torque != zeroV) {
        torques.push_back(f.torque);
    }
}

v3 P_State::net_torque() const {
    // sum of torques on object
    v3 net(zeroV);
    for (const auto& f: torques) {
        net += f;
    }
    return net;
}

void P_State::clear_torques() {
    torques.clear();
}

v3 P_State::net_force() const {
    // sum of forces on object
    v3 net(zeroV);
    for (const auto& f: forces) {
        net += f;
    }
    return net;
}

void P_State::clear_forces() {
    forces.clear();
}

std::ostream& operator<<(std::ostream& stream, const P_State& state) {
    stream << "Mass:" << state.mass << ", pos:" << printV(state.position)
    << ", velo:" << printV(state.velocity);
}
