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
#include "Util.hpp"

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
    tree_.insert(zeroV,id);
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
                collidingPairs.insert(colliding_pair);
            }
        }
    }
    
    for (const auto& p: collidingPairs) {
        const Id& id1 = p.first;
        const Id& id2 = p.second;
        const Actor& a1 = actors_[id1];
        const Actor& a2 = actors_[id2];
        const P_State& p1 = a1.get_state();
        const P_State& p2 = a2.get_state();

        // resolve collision
        std::cout << id1 << " and " << id2 << " colliding\n";

        const float m1 = p1.mass;
        const float m2 = p2.mass;
        const v3 mom_before = p1.momentum + p2.momentum;
        std::cout << "mom before " << printV(mom_before) << "\n";
        const v3 u_delta = p2.velocity - p1.velocity;
        const v3 ud_e = u_delta * restitution;
        const v3 v2 = (mom_before + m1 * ud_e) / (m1 + m2);
        // don't change order of these 2 lines
        const v3 v1 = (mom_before - (m2 * v2)) / m1;

        const v3 mom_1 = m1 * v1;
        const v3 mom_2 = m2 * v2;
        const v3 mom_1_delta = mom_1 - p1.momentum;
        const v3 mom_2_delta = mom_2 - p2.momentum;
        std::cout << "Force on " << id1 << " " << printV(mom_1_delta) << "\n";
        std::cout << "Force on " << id2 << " " << printV(mom_2_delta) << "\n";
        actors_.apply_force_abs(id1, mom_1_delta);
        actors_.apply_force_abs(id2, mom_2_delta);
    }
}

void World::apply_force(const Id& id, const v3& force) {
    actors_[id].apply_force(force);
}

void World::apply_force(const Id& id, const v3& force, const v3& point) {
    actors_[id].apply_force(force,point);
}

void World::apply_torque(const Id& id, const v3& force) {
    actors_[id].apply_torque(force);
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
