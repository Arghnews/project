#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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

World::World(v2 windowSize) :
    windowSize(windowSize) {
}

Actors& World::actors() {
    return actors_;
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

        if (!actor.invis()) {

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

void World::simulate(const float& t, const float& dt) {
    for (auto& a: actors_.underlying()) {
        P_State& cube_phys = (*a.second).state_to_change();
        phys_.integrate(cube_phys, t, dt);
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
