#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <map>
#include <utility>
#include <set>

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
#include <cmath>
#include <algorithm>

#include "MTV.hpp"
#include "World.hpp"

World::World(float worldSize, v2 windowSize, float restitution) :
    tree_(zeroV,worldSize),
    windowSize(windowSize),
    restitution(restitution) {
}

Actors& World::actors() {
    return actors_;
}

void World::insert(Actor* a) {
    const Id id = a->id;
    tree_.insert(a->get_state().position,id);
    actors_.insert(id, a);
}

void World::firedShot(const Id& id) {

    const v3 org = actors_[id].get_state().position;
    const v3 dir = actors_[id].get_state().facing();
    std::cout << id << " fired\n";

    // assume shooting at 1 for now
    const L_Cuboid& lc = actors_[1].logical_cuboid();
    const vv3& verts = lc.vertices;
    const int size = verts.size();
    for (int i=0; i<size; i+=4) {
        
    }

}

void World::simulate(const float& t, const float& dt) {
    // whenever move need to update octree
    for (auto& a: actors_.underlying()) {
        const Id& id = a.first;
        // don't similate immobile objects
        if (!a.second->mobile) {
            continue;
        }
        P_State& cube_phys = (*a.second).state_to_change();
        const bool deleted = tree_.del(cube_phys.position);
        assert(deleted && "Should always be able to delete last cube position");
        phys_.integrate(cube_phys, t, dt);
        tree_.insert(cube_phys.position, id);
    }
}

void World::collisions() {

    std::set<MTV> collidingPairs; // stuff that just started colliding
    //static std::map<MTV,int> alreadyColliding; // stuff that was already c
    static std::map<MTV,int> alreadyColliding; // stuff that was already colliding, for how long (ticks)
    std::set<std::pair<Id, Id>> pairsThisPass;

    long t_earlier = timeNowMicros();
    long a_total = 0l;
    long b_total = 0l;
    long c_total = 0l;
    for (const auto& a: actors_.underlying()) {
        const Id& id = a.first;
        Actor& actor = *a.second;
        const L_Cuboid& l_cub = actor.logical_cuboid();
        const P_State& phys = actor.get_state();
        long a_start = timeNowMicros();
        const vId nearby = tree_.queryRange(phys.position, l_cub.furthestVertex);
        ////std::cout << l_cub.furthestVertex << "\n";
        a_total += timeNowMicros() - a_start;
        int nearbys = 0;
        long b_start = timeNowMicros();
        for (const auto& b: nearby) {
            const Id& id_nearby = b;
            // collision with self
            if (id_nearby == id) {
                continue;
            }
            Actor& actor_nearby = actors_[id_nearby];
            // both actors immobile - ie. worldly blocks that don't move
            if (!actor.mobile && !actor_nearby.mobile) {
                continue;
            }
            ++nearbys;
            const L_Cuboid& l_cub = actor.logical_cuboid();
            const L_Cuboid& l_cub_nearby = actor_nearby.logical_cuboid();
            long c_start = timeNowMicros();
            MTV mtv = L_Cuboid::colliding(l_cub,l_cub_nearby);
            c_total += timeNowMicros() - c_start;
            const bool areColliding = mtv.colliding;
            mtv.id1 = std::min(id, id_nearby);
            mtv.id2 = std::max(id, id_nearby);
            auto paired = std::make_pair(mtv.id1,mtv.id2);
            // should move above collision check
            if (contains(pairsThisPass, paired)) {
                continue;
            } else {
                pairsThisPass.insert(paired);
            }
            if (areColliding) {
                // if not already colliding
                // add to justColliding
                collidingPairs.insert(mtv);
               //std::coutout << "Now colliding inserting" << mtv << "\n";
            } else {
                // if not colliding, erase from already colliding set
                if (contains(alreadyColliding, mtv)) {
                    auto size = alreadyColliding.size();
                    alreadyColliding.erase(mtv);
                   //std::coutout << "No longer colliding deleting " << mtv << "\n";
                    assert(alreadyColliding.size() + 1 == size);
                }
            }
        }
        b_total += timeNowMicros() - b_start;
    }
    //std::cout << "Total time of octree lookup " << ((double)a_total)/1000.0 << "ms" << "\n";
    //std::cout << "Total time of b " << ((double)b_total)/1000.0 << "ms" << "\n";
    //std::cout << "Total time of collision checking " << ((double)c_total)/1000.0 << "ms" << "\n";
    long taken = timeNowMicros() - t_earlier;
    //std::cout << "Time taken " << (double)taken/1000.0 << "ms for finding collisions" << "\n";
    
    std::map<Id,std::vector<Force>> forceQueue;
    
    taken = timeNowMicros();
    for (const auto& mtv_original: collidingPairs) {
        MTV mtv = mtv_original;

        mtv.axis = glm::normalize(mtv.axis);
        const Id& id1 = mtv.id1;
        const Id& id2 = mtv.id2;
        Actor& a1 = actors_[id1];
        Actor& a2 = actors_[id2];
        const P_State& p1 = a1.get_state();
        const P_State& p2 = a2.get_state();

       //std::coutout << id1 << " colliding with " << id2 << "\n";

        // -------------------------------------------------------- READ ME :
        // maybe consider case just using momentum resolve and solving sticking

        // resolve collision
        ////std::cout << id1 << " and " << id2 << " colliding\n";

        const float m1 = p1.mass;
        const float m2 = p2.mass;
        const v3 u1 = p1.velocity;
        const v3 u2 = p2.velocity;
        const float CR = 0.9f; // coef of restitution
        // honestly just don't change CR

        const v3 v1 = (m1*u1 + m2*u2 + m2*CR*(u2-u1)) / (m1+m2);
        const v3 v2 = (m1*u1 + m2*u2 + m1*CR*(u1-u2)) / (m1+m2);

        P_State& p_1 = a1.state_to_change();
        P_State& p_2 = a2.state_to_change();
        const v3 mom1 = m1 * v1;
        const v3 mom2 = m2 * v2;
        //if (contains(alreadyColliding, mtv)) {
        //std::coutout << "Already colliding " << mtv.id1 << " and " << mtv.id2 << "\n";
        //std::coutout << "From " << id1 << " mom:" << printV(m1*u1) << " and " << id2 << " mom:" << printV(m2*u2) << "\n";
        //std::coutout << "To " << id1 << " mom:" << printV(mom1) << " and " << id2 << " mom:" << printV(mom2) << "\n";
        const float small = 0.0001f;
        const float d1 = glm::length(glm::distance(p_1.position + mtv.axis * small, p_2.position));
        const float d2 = glm::length(glm::distance(p_1.position - mtv.axis * small, p_2.position));
        if (d2 < d1) {
            mtv.axis = -1.0f * mtv.axis;
            //std::coutout << "Inverting axis\n";
        }

        p_1.set_momentum(mom1);
        p_2.set_momentum(mom2);

        float overlap = mtv.overlap;
        assert(mtv.overlap >= 0.0f);

        //std::cout << mtv.overlap << "\n";
        if (mtv.overlap <= 0.045f) continue;

        overlap *= 0.3f;

        v3 f = mtv.axis * overlap;
        
        const float total_mass = m1+m2;
        const float total_mass_i = 1.0f/total_mass;
        const float m1_ratio = m1 * total_mass_i;
        const float m2_ratio = m2 * total_mass_i;
        float f1_m_mul = total_mass * m2_ratio;
        float f2_m_mul = total_mass * m1_ratio;

        auto velo_changer = [&] (const v3& u, const v3& v) -> float {
            float velo_change = glm::dot(glm::normalize(u),glm::normalize(v));
            if (!std::isnan(velo_change)) {
                //velo_change = std::fabs(velo_change);
                if (velo_change < 0.0f) {
                    velo_change *= 1.5f;
                }
                velo_change = std::fabs(velo_change);
            } else {
                velo_change = 1.0f;
            }
            return velo_change;
        };

        f1_m_mul *= velo_changer(u1,v1);
        f2_m_mul *= velo_changer(u2,v2);

        v3 f1 = f * f1_m_mul;
        v3 f2 = -f * f2_m_mul;
       //std::coutout << "ids/forces " << id1 << " " << printV(f1) << ", " << id2 << " " << printV(f2) << "\n";
        forceQueue[id1].push_back(Force(f1,Force::Type::Force,false,true));
        forceQueue[id2].push_back(Force(f2,Force::Type::Force,false,true));

        //std::cout << "Mtv: " << printV(mtv.axis) << " and overlap " << mtv.overlap << "\n";
        

        // NOTE : can swap m1 and m2 in the forces to cause a light thing flying into a heavy thing
        // to have the light thing fly back proportional to mass of the other
        // with them this way round ie. the force applied to each object is proportional to the other's mass
        // a better sense is achieved of the lighter object coming off worse in a collision
    }

    for (const auto& id_force: forceQueue) {
        const Id& id = id_force.first;
        const std::vector<Force>& forces = id_force.second;
        // all the forces on that actor
        Force force(forces[0]);
        if (forces.size() > 1) {
            for (int i=1; i<forces.size(); ++i) {
                force.force = force.force + forces[i].force; // sum
                // can decide what to do with multiple forces on object here
            }
           //std::coutout << "Dividing force - " << printV(force.force) << " on " << id << " by " << forces.size() << "\n";
           //std::coutout << "Divided force now " << printV(force.force) << "\n";
        }
        actors_.apply_force(id, force);
    }

    for (const auto& mtv: collidingPairs) {
       //std::coutout << "Copying " << a.id1 << "," << a.id2 << " to alreadyColliding\n";
        if (contains(alreadyColliding, mtv)) {
            alreadyColliding[mtv] = ++alreadyColliding[mtv];
        } else {
            alreadyColliding.insert(std::make_pair(mtv,0));
        }
    }

    if (collidingPairs.size() > 0) {
       //std::coutout << "\n";
    } else {
    }
    //long taken = timeNowMicros() - t;
    //std::cout << "Time taken for collision resolving " << (double)(timeNowMicros() - taken)/1000.0 << "ms" << "\n";
}

void World::apply_force(const Id& id, const Force& force) {
    actors_.apply_force(id,force);
}

void World::render() {
    const Actor& selectedActor = actors_.selectedActor();
    const m4 view = selectedActor.viewMatrix();
    const G_Cuboid& cam_graphical_cube = selectedActor.graphical_cuboid();
    const float aspectRatio = windowSize.x / windowSize.y;
    const GLuint viewLoc = glGetUniformLocation(cam_graphical_cube.shaderProgram(), "view");

    for (const auto& a: actors_.underlying()) {
        const G_Cuboid& graphical_cube = (*a.second).graphical_cuboid();
        const Actor& actor = (*a.second);

        if (true) {
        //if (!actor.invis()) {

            graphical_cube.bindBuffers();
            graphical_cube.useShader();

            m4 projection = glm::perspective(glm::radians(90.0f), aspectRatio, 0.1f, 200.0f);
            GLuint projectionLoc = glGetUniformLocation(graphical_cube.shaderProgram(), "projection");

            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

            glBindVertexArray(graphical_cube.VAO);

            m4 model = actor.modelMatrix();
            GLuint modelLoc = glGetUniformLocation(graphical_cube.shaderProgram(), "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, graphical_cube.drawSize());

            glBindVertexArray(0);
            glUseProgram(0);
            // Render end -- -- --
        }
    }
}
