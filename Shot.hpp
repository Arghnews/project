#ifndef SHOT_HPP
#define SHOT_HPP

#include <glm/glm.hpp>

#include "cereal/archives/portable_binary.hpp"
#include "Archiver.hpp"
#include "Util.hpp"

struct Shot {
    Id shooter;
    Shot() : Shot(-1,v3(0.0f),v3(0.0f)) {}
    Shot(const Id& shooter, const v3& org, const v3& dir) :
        shooter(shooter), org(org), dir(dir), hit(false) {}
    Id target;
    v3 org; // where id was when fired
    v3 dir; // id's direction of shot
    v3 hit_pos; // world coord of the shot's position
    bool hit;
    template<class Archive>
        void serialize(Archive& archive) {
            archive(shooter, target,
                org, dir, hit_pos, hit);
        }
};

#endif
