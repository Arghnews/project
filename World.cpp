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
#include "Octtree.hpp"
#include "AABB.hpp"
#include "Force.hpp"
#include "Util.hpp"
#include <cmath>
#include <algorithm>

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
    // whenever move need to update octtree
    for (auto& a: actors_.underlying()) {
        const Id& id = a.first;
        P_State& cube_phys = (*a.second).state_to_change();
        const bool deleted = tree_.del(cube_phys.position, id);
        assert(deleted && "Should always be able to delete last cube position");
        phys_.integrate(cube_phys, t, dt);
        tree_.insert(cube_phys.position, id);
    }
}

void World::collisions() {

    static std::set<std::pair<Id,Id>> alreadyColliding;

    // stuff that just collided
    std::set<std::pair<Id,Id>> collidingPairs;

    for (auto& a: actors_.underlying()) {
        const Id& id = a.first;
        Actor& actor = *a.second;
        const L_Cuboid& l_cub = actor.logical_cuboid();
        const P_State& phys = actor.get_state();
        const vv3Id nearby = tree_.queryRange(phys.position, 2.0f*l_cub.furthestVertex);
        for (const auto& b: nearby) {
            const v3& pos_nearby = b.first;
            const Id& id_nearby = b.second;
            if (id_nearby == id) {
                continue;
            }
            Actor& actor_nearby = actors_[id_nearby];
            //std::cout << "at" << printV(actors_[id].get_state().position) << "\n";
            //std::cout << "at" << printV(actors_[id_nearby].get_state().position) << "\n";
            const L_Cuboid& l_cub = actor.logical_cuboid();
            const L_Cuboid& l_cub_nearby = actor_nearby.logical_cuboid();
            const bool areColliding = L_Cuboid::colliding(l_cub,l_cub_nearby);
            if (areColliding) {
                const std::pair<Id,Id> colliding_pair = std::make_pair(
                        std::min(id,id_nearby),std::max(id,id_nearby));
                if (true || !contains(alreadyColliding, colliding_pair)) {
                    collidingPairs.insert(colliding_pair);
                }
                alreadyColliding.insert(colliding_pair);
            }
        }
    }

    for (const auto& p: collidingPairs) {
        const Id& id1 = p.first;
        const Id& id2 = p.second;
        Actor& a1 = actors_[id1];
        Actor& a2 = actors_[id2];
        const P_State& p1 = a1.get_state();
        const P_State& p2 = a2.get_state();

        // resolve collision
        std::cout << id1 << " and " << id2 << " colliding\n";

        const float m1 = p1.mass;
        const float m2 = p2.mass;
        std::cout << "Mass of " << id1 << " " << m1 << " and " << id2 << " " << m2 << "\n";
        const v3 u1 = p1.velocity;
        const v3 u2 = p2.velocity;
        std::cout << "Start_velocityof " << id1 << " " << printV(u1) << " and " << id2 << " " << printV(u2) << "\n";

        const v3 relativeDir = glm::normalize(p2.position - p1.position);
        const v3 myDir = glm::normalize(u1-u2);
        const float angle = glm::dot(myDir,relativeDir);
        //std::cout << "Angle " << angle << "\n";
        if (angle <= 0.0f) { // if moving away
            continue;
            std::cout << "Skipped\n";
        }

        const float rest = 0.0f;
        const v3 ue = (u2 - u1) * rest;
        std::cout << "Ue " << printV(ue) << "\n";
        const v3 b = m1*u1 + m2*u2;
        std::cout << "Total mom before " << printV(b) << "\n";

        const v3 v1 = (b - m2*ue) / (m1+m2);
        const v3 v2 = (b - m1*v1) / m2;
        //const v3 v2 = (b + m1*du_e) / (m1+m2);
        //const v3 v1 = (b - m2*v2) / m1;
        P_State& p_1 = a1.state_to_change();
        P_State& p_2 = a2.state_to_change();
        const float bla = 1.0f;
        auto mom1 = m1 * v1 * bla;
        auto mom2 = m2 * v2 * bla;
        std::cout << "Total mom after " << printV(mom1+mom2) << "\n";
        std::cout << "End_velocity of " << id1 << " " << printV(v1) << " and " << id2 << " " << printV(v2) << "\n";
        std::cout << "MOms were " << id1 << " mom  " << printV(m1*u1) << " and " << id2 << " " << printV(m2*u2) << "\n";
        std::cout << "Setting " << id1 << " mom to " << printV(mom1) << " and " << id2 << " " << printV(mom2) << "\n";
        p_1.momentum = mom1;
        p_2.momentum = mom2;
        //p_1.velocity = v1;
        //p_2.velocity = v2;
        p_1.recalc();
        p_2.recalc();

        //const float factor = 1.0f;
        //const v3 f1 = m1 * (v1 - u1) * factor;
        //const v3 f2 = m2 * (v2 - u2) * factor;
        //std::cout << "Force on " << id1 << " " << printV(f1) << "\n";
        //std::cout << "Force on " << id2 << " " << printV(f2) << "\n";
        //actors_.apply_force(id1, Force(f1,Force::Type::Force,false,false));
        //actors_.apply_force(id2, Force(f2,Force::Type::Force,false,false));
    }
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
