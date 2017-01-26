#ifndef STATE_H
#define STATE_H 
#include <sstream>
#include <iostream>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Util.hpp"

struct State {
    fq orient;
    v3 pos;
    v3 topCenter;
    v3 rotation;
    State();
    State(const State& s);
    State& operator=(const State& other);
    friend std::ostream& operator<<(std::ostream& stream, const State& state);
};
#endif
