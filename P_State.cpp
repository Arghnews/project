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
    forces.push_back(f);
}

const Forces& P_State::net_forces() const {
    return forces;
}

void P_State::clear_forces() {
    forces.clear();
}

std::ostream& operator<<(std::ostream& stream, const P_State& state) {
    stream << "Mass:" << state.mass << ", pos:" << printV(state.position)
    << ", velo:" << printV(state.velocity);
}
