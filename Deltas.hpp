#ifndef DELTAS_HPP
#define DELTAS_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Archiver.hpp"
#include "Util.hpp"

struct Mom_Pos {
    v3 momentum;
    v3 position;
    Id id;
    template<class Archive>
        void serialize(Archive& archive) {
            archive(momentum, position, id);
        }
    Mom_Pos(const v3& mom, const v3& pos, const Id& id) :
         momentum(mom), position(pos), id(id) {}
    Mom_Pos() {}
};

struct AngMom_Ori {
    v3 ang_momentum;
    Id id;
    fq orient;
    template<class Archive>
        void serialize(Archive& archive) {
            archive(ang_momentum, id, orient);
        }
    AngMom_Ori(const v3& ang_mom, const fq& ori, const Id& id) :
        ang_momentum(ang_mom), orient(ori), id(id) {}
    AngMom_Ori() {}
};

#endif
