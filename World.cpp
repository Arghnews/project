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
    const Id id = actors_.insert(a);
    tree_.insert(a->get_state().position,id);
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
                std::cout << "Now colliding inserting" << mtv << "\n";
            } else {
                // if not colliding, erase from already colliding set
                if (contains(alreadyColliding, mtv)) {
                    auto size = alreadyColliding.size();
                    alreadyColliding.erase(mtv);
                    std::cout << "No longer colliding deleting " << mtv << "\n";
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
    

    
    taken = timeNowMicros();
    for (const auto& mtv_original: collidingPairs) {
        MTV mtv = mtv_original;

        if (contains(alreadyColliding, mtv)) {
            std::cout << "Already colliding skipping " << mtv.id1 << " and " << mtv.id2 << "\n";
        }

        mtv.axis = glm::normalize(mtv.axis);
        const Id& id1 = mtv.id1;
        const Id& id2 = mtv.id2;
        Actor& a1 = actors_[id1];
        Actor& a2 = actors_[id2];
        const P_State& p1 = a1.get_state();
        const P_State& p2 = a2.get_state();

        std::cout << id1 << " colliding with " << id2 << "\n";

        // -------------------------------------------------------- READ ME :
        // maybe consider case just using momentum resolve and solving sticking

        // resolve collision
        ////std::cout << id1 << " and " << id2 << " colliding\n";

        const float m1 = p1.mass;
        const float m2 = p2.mass;
        const v3 u1 = p1.velocity;
        const v3 u2 = p2.velocity;
        const float CR = 1.0f; // coef of restitution

        const v3 v1 = (m1*u1 + m2*u2 + m2*CR*(u2-u1)) / (m1+m2);
        const v3 v2 = (m1*u1 + m2*u2 + m1*CR*(u1-u2)) / (m1+m2);

        //////std::cout << "Angle " << angle << "\n";
        //const float angle = glm::dot(myDir,relativeDir);
            ////std::cout << "Skipped - angle " << angle << "\n";
            //continue;

        ////std::cout << "Start_velocityof " << id1 << " " << printV(u1) << " and " << id2 << " " << printV(u2) << "\n";
        ////std::cout << "Ue " << printV(ue) << "\n";
        ////std::cout << "Total mom before " << printV(b) << "\n";

        //const v3 v2 = (b + m1*du_e) / (m1+m2);
        //const v3 v1 = (b - m2*v2) / m1;
        ////std::cout << "Total mom after " << printV(mom1+mom2) << "\n";
        ////std::cout << "End_velocity of " << id1 << " " << printV(v1) << " and " << id2 << " " << printV(v2) << "\n";
        ////std::cout << "MOms were " << id1 << " mom  " << printV(m1*u1) << " and " << id2 << " " << printV(m2*u2) << "\n";
        //std::cout << "Setting " << id1 << " mom to " << printV(mom1) << " and " << id2 << " " << printV(mom2) << "\n";
        P_State& p_1 = a1.state_to_change();
        P_State& p_2 = a2.state_to_change();
        const v3 mom1 = m1 * v1;
        const v3 mom2 = m2 * v2;
        p_1.set_momentum(mom1);
        p_2.set_momentum(mom2);
        std::cout << "From " << id1 << " mom:" << printV(m1*u1) << " and " << id2 << " mom:" << printV(m2*u2) << "\n";
        std::cout << "To " << id1 << " mom:" << printV(mom1) << " and " << id2 << " mom:" << printV(mom2) << "\n";
        const float small = 0.01f;
        const float d1 = glm::length(glm::distance(p_1.position + mtv.axis * small, p_2.position));
        const float d2 = glm::length(glm::distance(p_1.position - mtv.axis * small, p_2.position));
        if (d2 < d1) {
            mtv.axis = -1.0f * mtv.axis;
            std::cout << "Inverting axis\n";
        }
        
        v3 f = mtv.axis * (mtv.overlap + 0.01f);
        actors_.apply_force(id1,Force(f,Force::Type::Force,false,true));
        actors_.apply_force(id2,Force(-f,Force::Type::Force,false,true));

        //std::cout << "Mtv: " << printV(mtv.axis) << " and overlap " << mtv.overlap << "\n";
        

        // NOTE : can swap m1 and m2 in the forces to cause a light thing flying into a heavy thing
        // to have the light thing fly back proportional to mass of the other
        // with them this way round ie. the force applied to each object is proportional to the other's mass
        // a better sense is achieved of the lighter object coming off worse in a collision
    }

    for (const auto& a: collidingPairs) {
        std::cout << "Copying " << a.id1 << "," << a.id2 << " to alreadyColliding\n";
        alreadyColliding.insert(a);
    }

    if (collidingPairs.size() > 0) {
        std::cout << "\n";
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
