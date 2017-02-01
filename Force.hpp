#ifndef FORCE_HPP 
#define FORCE_HPP

#include <glm/glm.hpp>

struct Force {
    v3 force;
    bool relative;
    v3 torque;
    Force(const v3& force, const bool& relative) :
        Force(force,relative,zeroV) {
    }
    Force(const v3& force, const bool& relative, const v3& torque) :
        force(force),
        relative(relative),
        torque(torque) {
        }
};

#endif
