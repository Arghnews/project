#ifndef WORLD_HPP
#define WORLD_HPP

#include <glm/glm.hpp>
#include <deque>

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

#include "Shot.hpp"

class World {
    private:
        Actors actors_;
        Physics phys_;
        Octree tree_;
        float restitution;
        std::map<Id, v3> positions_;
        std::deque<Force> force_queue_;
        std::deque<Shot> shot_queue_;
    public:
        std::map<int,G_Cuboid> g_cubs;
        World(float worldSize, v2 windowSize, float restitution);
        v2 windowSize;
        Actors& actors();
        void insert(Actor* a);
        void simulate(const float& t, const float& dt);
        void apply_force(const Force& force);
        void apply_forces();
        void render();
        void collisions();
        void fire_shot(const Id& id);
        void fire_shots();

        void blow_up(const Id& id);
        std::vector<MTV> colliding_with(const Id& id);
        Shot shot_face(const vv3& verts24, const v3& org, const v3& dir, const int& i);
        Shot shot_actor(const v3& org, const v3& dir, const Id& id);
};

#endif
