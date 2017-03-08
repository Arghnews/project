#ifndef FORCE_HPP 
#define FORCE_HPP

#include <glm/glm.hpp>

#include "cereal/archives/portable_binary.hpp"
#include "Archiver.hpp"
#include "Util.hpp"

struct Force {
    enum class Type { Force, Torque };
    Id id;
    v3 force;
    Type t;
    bool relative; // motion all relative to actor or absolute
    bool affected; // ie. affected by air_res
    Force() {}
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
    template<class Archive>
        void serialize(Archive& archive) {
            archive(id, force, t, relative, affected);       
        }
};

#endif
