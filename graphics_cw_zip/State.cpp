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

State operator+(const State& s1, const State& s2) {
    State s;
    s.orient = s2.orient * s1.orient; // ORDER
    s.pos = s1.pos + s2.pos;
    s.topCenter = s1.topCenter + s2.topCenter;
    s.rotation = s1.rotation + s2.rotation;
}

std::ostream& operator<<(std::ostream& stream, const State& state) {
    return stream << "Pos" << printVec(state.pos) << ", topC:" << printVec(state.topCenter) << " and rotation:" << printVec(state.rotation);
}
