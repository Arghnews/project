#ifndef FORCE_HPP 
#define FORCE_HPP

#include <glm/glm.hpp>

#include "Util.hpp"

struct Force {
    enum class Type { Force, Torque };
    Id id;
    v3 force;
    Type t;
    bool relative; // motion all relative to actor or absolute
    bool affected; // ie. affected by air_res
    Force(const Id& id, const v3& force, Type t) :
        Force(id,force,t,true,true) {
    }
    Force(const Id& id, const v3& force, Type t, const bool& relative, const bool& affected) :
        id(id),
        force(force),
        t(t),
        relative(relative),
        affected(affected)
        {
        }
};

#endif
