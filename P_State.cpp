#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Util.hpp"
#include "P_State.hpp"

#include <iostream>
#include <sstream>

/* a simple class that encompasses the physical state of an object
 * Things like it's position, momentum, mass and orientation are here
*/

P_State::P_State(float m, v3 pos) :
    mass(m),
    inverse_mass(1.0f/m),
    position(pos) {
    }

void P_State::turn(const v3& v) {
    orient = v * orient;
}

void P_State::recalc() {
    velocity = momentum * inverse_mass;
    orient = glm::normalize(orient);
}

// forces are relative to objects facing direction
void P_State::add_force(const v3& f) {
    // order matters
    forces.push_back(f * orient);
}

// force is absolute, ie v(0,-1,0) is down
void P_State::add_force_abs(const v3& f) {
    forces.push_back(f);
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
    << ", velo:" << printV(state.velocity) << ", force:" << printV(state.force);
}
