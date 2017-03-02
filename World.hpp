#ifndef WORLD_HPP
#define WORLD_HPP

#include <glm/glm.hpp>

#include "Actors.hpp"
#include "Actor.hpp"
#include "Physics.hpp"
#include "G_Cuboid.hpp"
#include "L_Cuboid.hpp"
#include "P_State.hpp"
#include "Physics.hpp"
#include "Octree.hpp"
#include "AABB.hpp"
#include "Force.hpp"
#include "Util.hpp"

struct Hit {
    v3 pos;
    Id id;
    bool hit;
    Hit() : Hit(-1) {}
    Hit(const Id& id) : pos(0.0f), id(id), hit(false) {}
};

class World {
    private:
        Actors actors_;
        Physics phys_;
        Octree tree_;
        float restitution;
        std::map<Id, v3> positions_;
    public:
        World(float worldSize, v2 windowSize, float restitution);
        v2 windowSize;
        Actors& actors();
        void insert(Actor* a);
        void simulate(const float& t, const float& dt);
        void apply_force(const Id& id, const Force& force);
        void render();
        void collisions();
        void firedShot(const Id& id);

        Hit hit_face(const vv3& verts24, const v3& org, const v3& dir, const int& i);
        Hit hit_actor(const v3& org, const v3& dir, const Id& id);
};

#endif
