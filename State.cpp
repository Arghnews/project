#include <sstream>
#include <iostream>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Util.hpp"
#include "State.hpp"
#include <iostream>

State::State() {
}

State::State(const State& s) :
    orient(s.orient),
    pos(s.pos),
    topCenter(s.topCenter),
    rotation(s.rotation) {
}

State& State::operator=(const State& other) {
    if (this != &other) {
        orient = other.orient;
        pos = other.pos;
        topCenter = other.topCenter;
        rotation = other.rotation;
    }
    return *this;
}

std::ostream& operator<<(std::ostream& stream, const State& state) {
    return stream << "Pos" << printVec(state.pos) << ", topC:" << printVec(state.topCenter) << " and rotation:" << printVec(state.rotation);
}
