#ifndef WORLD_HPP
#define WORLD_HPP

#include <glm/glm.hpp>
#include <deque>

#include "Util.hpp"
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
#include "Deltas.hpp"

#include <memory>
#include "Shot.hpp"

class World {
    private:
        Actors actors_;
        Physics phys_;
        Octree tree_;
        Forces force_queue_;
        Shots shot_queue_;
        Shot shot_face(const vv3& verts24, const v3& org, const v3& dir, const int& i);
        Shot shot_actor(const v3& org, const v3& dir, const Id& id);
    public:
        std::map<int,std::unique_ptr<G_Cuboid>> g_cubs;
        std::map<int,vv3> l_cub_face_verts;
        World(float worldSize, v2 windowSize);
        v2 windowSize;
        Actors& actors();
        //void insert(Actor* a);
        void insert(Actor a);
        void simulate(
                const float& t,
                const float& dt,
                std::vector<Mom_Pos>& vec_mom_pos,
                std::vector<AngMom_Ori>& vec_angmom_ori
                );
        void apply_force(const Force& force);
        void apply_forces(const Forces& forces);
        void clear_forces();
        void clear_shots();
        Forces& forces();
        Shots& shots();
        void fire_shot(const Id& id);
        void fire_shot(const Shot& shot);
        Shots fire_shots(const Shots& shots);

        void render();
        void collisions();

        void blow_up(const Id& id);
        std::vector<MTV> colliding_with(const Id& id);
};

#endif
