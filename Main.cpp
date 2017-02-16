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

//#include <xmmintrin.h>
//#include <pmmintrin.h>

/* TO DO
 - Need to have case for static on non static collide, so you can't push stuff through floor
 - Fixed octtree?
 - Modified octtree to use 128 per level instead of 1, huge perf. improve
 - For optim - consider caching more data, maybe never have to recompute verts? I dunno
 - Consider octtree reimplement with ordered maps
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

static const float my_mass = 3.0f;
static const float other_mass = 1.0f;
static const float small = my_mass * 0.01f;
static const long program_start_time = timeNowMicros();

long static timeNow() {
    return timeNowMicros() - program_start_time;
}

int main() {
//_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    ////_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    Window_Inputs inputs;

    GLFWwindow* window = inputs.init_window(1440, 1024);

    const long fps_max = 60l;
    
    const long tickrate = 100l;
    const long dt = (1e6l)/tickrate; // run at tickrate 100

    long t = 0l;
    long currentTime = timeNow();
    long acc = 0l;

    const float areaSize = 2000.0f;

    const float restitution = 0.05f;

    World world(areaSize, inputs.windowSize() * 0.6f, restitution, 128);

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
    bool mobile
    */

    float scaleFactor = 1.0f;
    const v3 scale = scaleFactor * oneV;
    Actor* me = new Actor(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), scale,
            5.0f * oneV, my_mass, 5.0f, true, true);
    world.insert(me);

    Actor* cube1 = new Actor(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), scale,
            3.0f * oneV, other_mass, 5.0f, true, true);
    world.insert(cube1);

    Actor* cube2 = new Actor(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), scale,
            v3(0.0f,9.0f,5.0f), other_mass, 5.0f, true, true);
    world.insert(cube2);

    Actor* cube3 = new Actor(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), scale,
            v3(5.0f,8.0f,0.0f), other_mass, 5.0f, true, true);
    world.insert(cube3);

    static const float seperator = 1.03f;
    static const float floor_mass = 1.0f;
    const int n = 25;
    const int m = 10;
    for (int i=0; i<n; ++i) {
        for (int j=0; j<m; ++j) {
            const float ang_sep = 1.5f;
            const v3 position(ang_sep*scaleFactor*(seperator*(float)i-n/2), 0.0f, ang_sep*scaleFactor*(seperator*(float)j-m/2));
            Actor* floorpiece = new Actor(&vertices, "shaders/vertex.shader",
                    "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), scale,
                    position, floor_mass, 5.0f, false, true);
            world.insert(floorpiece);
            world.apply_force(std::max(world.actors().size()-1,0),Force(v3(i,0.0f,j),Force::Type::Torque));
        }
    }

    const float y_offset = -10.0f;
    for (int i=0; i<n; ++i) {
        for (int j=0; j<m; ++j) {
            const v3 position(scaleFactor*(seperator*(float)i-n/2), y_offset, scaleFactor*(seperator*(float)j-m/2));
            Actor* floorpiece = new Actor(&vertices, "shaders/vertex.shader",
                    "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), scale,
                    position, floor_mass, 5.0f, false, true);
            world.insert(floorpiece);
        }
    }

    const float other_y_offset = -20.0f;
    for (int i=0; i<n; ++i) {
        for (int j=0; j<m; ++j) {
            const v3 position(scaleFactor*(seperator*(float)i-n/2), other_y_offset, scaleFactor*(seperator*(float)j-m/2));
            Actor* floorpiece = new Actor(&vertices, "shaders/vertex.shader",
                    "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), scale,
                    position, floor_mass, 5.0f, false, true);
            world.insert(floorpiece);
        }
    }

    const float other_other_y_offset = -30.0f;
    for (int i=0; i<n; ++i) {
        for (int j=0; j<m; ++j) {
            const v3 position(scaleFactor*(seperator*(float)i-n/2), other_other_y_offset, scaleFactor*(seperator*(float)j-m/2));
            Actor* floorpiece = new Actor(&vertices, "shaders/vertex.shader",
                    "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), scale,
                    position, floor_mass, 5.0f, false, false);
            world.insert(floorpiece);
        }
    }

    const int nt = 0;
    for (int i=1; i<nt+1; ++i) {
        const v3 position(0.0f, scaleFactor*(seperator*(float)i), 0.0f);
        Actor* floorpiece = new Actor(&vertices, "shaders/vertex.shader",
                "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), scale,
                position, floor_mass, 5.0f, false, false);
        world.insert(floorpiece);
    }
    /*Actor* the_floor = new Actor(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), v3(oneV),
            zeroV, floor_mass, 5.0f, false);
    world.insert(the_floor);*/

    set_keyboard(inputs,window,world.actors());

    static int frame = 0;

    while (!glfwWindowShouldClose(window)) {
        ++frame;
        std::cout << "Start of frame " << frame << "\n";
        const long newTime = timeNow();
        const long frameTime = newTime - currentTime;
        currentTime = newTime;
        acc += std::min(frameTime,1000l*100l);
        //acc += frameTime;
        std::cout << "frameTIme" << frameTime << "\n";

        // simulate world
        int runs = 0;
        long worldSims = 0l;
        long worldCols = 0l;
        while (acc >= dt) {
            ++runs;
            // process inputs, change world
            inputs.processInput(); // polls input and executes action based on that

            const v2 mouseDelta = inputs.cursorDelta();
            world.apply_force(world.actors().selected(),Force(v3(glm::radians(mouseDelta.y), glm::radians(mouseDelta.x), 0.0f),Force::Type::Torque,false,false));

            static const float normalize = 1.0f / 1e4f;
            const float t_normalized = t * normalize;
            const float dt_normalized = dt * normalize;

            long marker = timeNow();
            world.simulate(t_normalized,dt_normalized);
            worldSims += (timeNow() - marker);

            marker = timeNow();
            world.collisions();
            worldCols += (timeNow() - marker);

            // feeds in essentially a time value of 1 every time
            // since fixed time step
            acc -= dt;
            t += dt;
        }
        //std::cout << "Time taken for world sim loop " << ((double)(worldSims)/1000.0) << "ms\n";
        //std::cout << "Time taken for world col loop " << ((double)(worldCols)/1000.0) << "ms\n";
        //std::cout << "Ran world sim loop " << runs << " times\n";
        // Render -- -- --
        // local space -> world space -> view space -> clip space -> screen space
        //          model matrix   view matrix  projection matrix   viewport transform
        gl_loop_start();

        long renderTime = timeNow();
        world.render();
        //std::cout << "Render time: " << (double)(timeNow() - renderTime) / 1000.0 << "ms\n";
        
        // sleep if fps would be > fps_max
        long spareFrameTime = 1e6l / fps_max - (timeNow() - newTime);
        if (spareFrameTime < 3000l) {
            //std::cout << "---\t---- Spare frame time " << spareFrameTime << "---\t---\n";
        }
        //std::this_thread::sleep_for(std::chrono::microseconds(std::max(0l,spareFrameTime)));

        inputs.swapBuffers(); // swaps buffers

        long thisFrameTime = timeNow() - newTime;
        std::cout << "This frame: " << frame << ", frametime: " << (double)thisFrameTime/1000.0 << "ms\n";
        std::cout << "End of frame " << frame << "\n";
    }

    inputs.close();
}

void gl_loop_start() {
    // Clear the colorbufer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
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

    inputs.setFunc2(GLFW_KEY_P,[&] () {
            //actors.apply_force(actors.selected(),Force(LEFT,Force::Type::Torque,false,true));
            // resets actor's roll
            actors.selectedActor().reorient();
    });
    inputs.setFunc2(GLFW_KEY_R,[&] () {
            actors.apply_force(actors.selected(),Force(LEFT,Force::Type::Torque,false,true));
    });
    inputs.setFunc2(GLFW_KEY_Y,[&] () {
            actors.apply_force(actors.selected(),Force(UP,Force::Type::Torque,false,true));
    });
    inputs.setFunc2(GLFW_KEY_Z,[&] () {
            actors.apply_force(actors.selected(),Force(FORWARD,Force::Type::Torque,false,true));
    });
    
            /*
            const v3 pos = actors.selectedActor().get_state().position;
            const fq orient = actors.selectedActor().get_state().orient;
            const v3 f = (orient * v3(0.5f,0.0f,0.0f)) + pos;
            actors.apply_force(actors.selected(),FORWARD,f);
            */
    //});
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
