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
#include "Util.hpp"

class World {
    private:
        Actors actors_;
        Physics phys_;
    public:
        World(v2 windowSize=v2(1024.0f,768.0f));
        v2 windowSize;
        Actors& actors();
        void simulate(const float& t, const float& dt);
        void apply_force(const Id& id, const v3& force);
        void apply_force(const Id& id, const v3& force, const v3& point);
        void apply_torque(const Id& id, const v3& force);
        void render();
};

#endif
