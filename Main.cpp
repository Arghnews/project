#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <algorithm>
#include <string>
#include <map>
#include <iterator>

#include "Util.hpp"
#include "Shader.hpp"
#include "Window_Inputs.hpp"
#include "G_Cuboid.hpp"
#include "data.hpp"
#include "Actor.hpp"
#include "Physics.hpp"
#include "Actors.hpp"
#include "World.hpp"
#include "Force.hpp"

/* TO DO
 - Try actually set velocities/orients
 - Velocities before and after in conserve of momentum are too similiar
 - Consider merging P_State and L_Cuboid, they are becoming too dependent on each other
 - Collisions resolve
 - Collision where they hit
 - Gravity
 - Perhaps camera?
 - Movement caching
*/

void gl_loop_start();
void set_keyboard(Window_Inputs& inputs, GLFWwindow* window, Actors& actors);
void select_cube(Window_Inputs& inputs, Actors& actors);

static const float my_mass = 10.0f;
static const float other_mass = 1.0f;
static const float small = my_mass * 0.01f;

int main() {

    Window_Inputs inputs;

    GLFWwindow* window = inputs.init_window(1440, 1024);

    const long fps_max = 60l;
    
    const long tickrate = 100l;
    const long dt = (1e6l)/tickrate; // run at tickrate 100

    auto timeNow = [] () -> long {
        static const long program_start_time = timeNowMicros();
        return timeNowMicros() - program_start_time;
    };
    long t = 0l;
    long currentTime = timeNow();
    long acc = 0l;

    const float areaSize = 500.0f;

    const float restitution = 0.05f;
    World world(areaSize, inputs.windowSize() * 0.6f, restitution);

    /*
    const fv* vertexData,
    std::string vertShader,
    std::string fragShader,
    v3 topCenter,
    v3 scale,
    v3 startPos,
    float mass,
    float inertia,
    bool selectable
    */

    Actor* me = new Actor(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), oneV,
            5.0f * oneV, my_mass, 5.0f, true);
    world.insert(me);

    Actor* cube1 = new Actor(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), v3(2.0f,1.0f,2.0f),
            3.0f * oneV, other_mass, 5.0f, true);
    world.insert(cube1);

    Actor* cube2 = new Actor(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), v3(1.0f,1.0f,1.0f),
            v3(0.0f,-3.0f,0.0f), other_mass, 5.0f, true);
    world.insert(cube2);

    /*
    Actor* cube3 = new Actor(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), v3(1.0f,1.0f,1.0f),
            v3(5.0f,-3.0f,0.0f), 0.0001f, 5.0f, true);
    world.insert(cube3);
    */
    /*
    Actor* the_floor = new Actor(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), v3(areaSize/2.0f,0.1f,areaSize/2.0f),
            zeroV, 10.0f, 5.0f, false);
    world.insert(the_floor);
    */

    set_keyboard(inputs,window,world.actors());

    while (!glfwWindowShouldClose(window)) {

        long newTime = timeNow();
        long frameTime = newTime - currentTime;
        currentTime = newTime;
        acc += frameTime;

        // simulate world
        while (acc >= dt) {
            // process inputs, change world
            inputs.processInput(); // polls input and executes action based on that

            const v2 mouseDelta = inputs.cursorDelta();
            world.apply_force(world.actors().selected(),Force(v3(glm::radians(mouseDelta.y), glm::radians(mouseDelta.x), 0.0f),Force::Type::Torque,false,false));

            static const float normalize = 1.0f / 1e4f;
            const float t_normalized = t * normalize;
            const float dt_normalized = dt * normalize;
            std::cout << dt_normalized << "\n";
            world.simulate(t_normalized,dt_normalized);
            world.collisions();

            // feeds in essentially a time value of 1 every time
            // since fixed time step
            acc -= dt;
            t += dt;
        }

        // Render -- -- --
        // local space -> world space -> view space -> clip space -> screen space
        //          model matrix   view matrix  projection matrix   viewport transform
        gl_loop_start();

        world.render();
        
        // sleep if fps would be > fps_max
        long spareFrameTime = 1e6l / fps_max - (timeNow() - newTime);
        std::this_thread::sleep_for(std::chrono::microseconds(std::max(0l,spareFrameTime)));

        inputs.swapBuffers(); // swaps buffers
    }

    inputs.close();
}

void gl_loop_start() {
    // Clear the colorbufer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void set_keyboard(Window_Inputs& inputs, GLFWwindow* window, Actors& actors) {
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_PRESS,[&] () {std::cout << "You pressed escape\n"; });
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_REPEAT,[&] () {std::cout << "You held escape\n"; });

    inputs.setFunc(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, [&] () {std::cout << "Left mouse\n"; });

    // must be capture by value here
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_RELEASE,[=] () {glfwSetWindowShouldClose(window, GLFW_TRUE); });

    inputs.setFunc1(GLFW_KEY_TAB,[&] () {
        actors.next(); 
        select_cube(inputs,actors);        
    });
}

void select_cube(Window_Inputs& inputs, Actors& actors) {
    // camera

    inputs.setFunc2(GLFW_KEY_R,[&] () {
            actors.apply_force(actors.selected(),Force(LEFT,Force::Type::Torque));
    });
    inputs.setFunc2(GLFW_KEY_Y,[&] () {
            actors.apply_force(actors.selected(),Force(UP,Force::Type::Torque));
    });
    inputs.setFunc2(GLFW_KEY_Z,[&] () {
            actors.apply_force(actors.selected(),Force(FORWARD,Force::Type::Torque));
    });

    inputs.setFunc2(GLFW_KEY_P,[&] () {
            /*
            const v3 pos = actors.selectedActor().get_state().position;
            const fq orient = actors.selectedActor().get_state().orient;
            const v3 f = (orient * v3(0.5f,0.0f,0.0f)) + pos;
            actors.apply_force(actors.selected(),FORWARD,f);
            */
    });
    inputs.setFunc2(GLFW_KEY_W,[&] () {
            actors.apply_force(actors.selected(),Force(small*FORWARD,Force::Type::Force));
    });
    inputs.setFunc2(GLFW_KEY_S,[&] () {
            actors.apply_force(actors.selected(),Force(small*BACKWARD,Force::Type::Force));
    });
    inputs.setFunc2(GLFW_KEY_A,[&] () {
            actors.apply_force(actors.selected(),Force(small*LEFT,Force::Type::Force));
    });
    inputs.setFunc2(GLFW_KEY_D,[&] () {
            actors.apply_force(actors.selected(),Force(small*RIGHT,Force::Type::Force));
    });
}
