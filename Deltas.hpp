#ifndef DELTAS_HPP
#define DELTAS_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Archiver.hpp"
#include "Util.hpp"

struct Mom_Pos {
    v3 position;
    v3 momentum;
    Id id;
    template<class Archive>
        void serialize(Archive& archive) {
            archive(position, momentum, id);
        }
    Mom_Pos(const v3& pos, const v3& mom, const Id& id) :
        position(pos), momentum(mom), id(id) {}
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
