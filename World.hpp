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
#include "Octtree.hpp"
#include "AABB.hpp"
#include "Force.hpp"
#include "Util.hpp"

class World {
    private:
        Actors actors_;
        Physics phys_;
        Octtree tree_;
        float restitution;
    public:
        World(float worldSize, v2 windowSize, float restitution);
        v2 windowSize;
        Actors& actors();
        void insert(Actor* a);
        void simulate(const float& t, const float& dt);
        void apply_force(const Id& id, const Force& force);
        void render();
        void collisions();
};

#endif
