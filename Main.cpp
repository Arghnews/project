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
#include <sstream>

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
 - Fixed octree?
 - Modified octree to use 128 per level instead of 1, huge perf. improve
 - For optim - consider caching more data, maybe never have to recompute verts? I dunno
 - Consider octree reimplement with ordered maps
 - Consider merging P_State and L_Cuboid, they are becoming too dependent on each other
 - Collisions resolve
 - Collision where they hit
 - Gravity
 - Perhaps camera?
 - Movement caching
*/

void gl_loop_start();
void set_keyboard(Window_Inputs& inputs, GLFWwindow* window, World& world);
void select_cube(Window_Inputs& inputs, World& world);

static const float my_mass = 10.0f;
static const float cube1_mass = 1.0f;
static const float cube2_mass = 1.0f;
static const float cube3_mass = 1.0f;
static const float other_mass = 1.0f;
static const float small = my_mass * 0.05f;
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
    long lastFrameStart = timeNow();
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
    bool mobile
    */

    float scaleFactor = 1.0f;
    const v3 scale = scaleFactor * oneV;
    Actor* me = new Actor(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), v3(1.0f,1.0f,1.0f),
            5.0f * oneV, my_mass, 5.0f, true, true);
    world.insert(me);

    Actor* cube1 = new Actor(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), v3(1.0f,1.0f,1.0f),
            v3(0.0f,3.0f,1.0f), cube1_mass, 5.0f, true, true);
    world.insert(cube1);

    Actor* cube2 = new Actor(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), scale,
            v3(0.0f,3.0f,3.0f), cube2_mass, 5.0f, true, true);
    world.insert(cube2);

    Actor* cube3 = new Actor(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), scale,
            v3(0.0f,3.0f,5.0f), cube3_mass, 5.0f, true, true);
    world.insert(cube3);

    static const float seperator = 1.15f;
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
            std::cout << std::max(world.actors().size()-1,0) << "\n";
            world.apply_force(std::max(world.actors().size()-1,0),Force(v3(i,0.0f,j),Force::Type::Torque));
        }
    }

    const float y_offset = -10.0f;
    for (int i=0; i<n; ++i) {
        for (int j=0; j<m; ++j) {
            const v3 position(scaleFactor*(seperator*(float)i-n/2), y_offset, scaleFactor*(seperator*(float)j-m/2));
            Actor* floorpiece = new Actor(&vertices, "shaders/vertex.shader",
                    "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), scale,
                    position, 1.0f, 5.0f, false, true);
            world.insert(floorpiece);
        }
    }

    const float other_y_offset = -20.0f;
    for (int i=0; i<n; ++i) {
        for (int j=0; j<m; ++j) {
            const v3 position(scaleFactor*(seperator*(float)i-n/2), other_y_offset, scaleFactor*(seperator*(float)j-m/2));
            Actor* floorpiece = new Actor(&vertices, "shaders/vertex.shader",
                    "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), scale,
                    position, 5.0f, 5.0f, false, true);
            world.insert(floorpiece);
        }
    }

    const float other_other_y_offset = -30.0f;
    for (int i=0; i<n; ++i) {
        for (int j=0; j<m; ++j) {
            const v3 position(scaleFactor*(seperator*(float)i-n/2), other_other_y_offset, scaleFactor*(seperator*(float)j-m/2));
            Actor* floorpiece = new Actor(&vertices, "shaders/vertex.shader",
                    "shaders/fragment.shader", v3(0.0f,0.5f,0.0f), scale,
                    position, 25.0f, 5.0f, false, true);
            world.insert(floorpiece);
        }
    }

    set_keyboard(inputs,window,world);

    static int frame = 0;

    float fps = 60.0f;
    float f_time = 1e6f / fps;

    while (!glfwWindowShouldClose(window)) {
        ++frame;
        std::stringstream print_buffer;
        //std::cout << "Start of frame " << frame << "\n";
        long frameStart = timeNow();
        //const long frameTime = std::max(frameStart - lastFrameStart,f_time);
        const long frameTime = (long)f_time;
        lastFrameStart = frameStart;
        //auto extra = std::min(frameTime,1000l*100l);
        auto extra = frameTime;
        //acc += frameTime;
        print_buffer << "Adding to acc(" << acc << ")" << extra << "\n";
        acc += extra;
        //print_buffer << "frameTIme" << frameTime << "\n";

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
        print_buffer << "Time taken for simulations this loop " << ((double)(worldSims)/1000.0) << "ms\n";
        print_buffer << "Time taken for collisions this  loop " << ((double)(worldCols)/1000.0) << "ms\n";
        print_buffer << "Ran world sim loop " << runs << " times\n";
        // Render -- -- --
        // local space -> world space -> view space -> clip space -> screen space
        //          model matrix   view matrix  projection matrix   viewport transform
        gl_loop_start();

        long renderTime = timeNow();
        world.render();

        print_buffer << "Render time: " << (double)(timeNow() - renderTime) / 1000.0 << "ms\n";

        // sleep if fps would be > fps_max
        //print_buffer << "---\t---- Spare frame time " << spareFrameTime << "---\t---\n";
        //std::this_thread::sleep_for(std::chrono::microseconds(std::max(0l,spareFrameTime)));

        inputs.swapBuffers(); // swaps buffers

        double thisFrameTime = (timeNow() - frameStart)/1000.0;
        print_buffer << "This frame: " << frame << ", frametime: " << thisFrameTime << "ms\n";
        print_buffer << "End of frame " << frame << "\n";
        if (thisFrameTime > 10.0) {
            //std::cout << print_buffer.str();
            print_buffer << std::endl;
            //std::cout << "This frame: " << frame << ", frametime: " << thisFrameTime << "ms\n";
        }
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

void set_keyboard(Window_Inputs& inputs, GLFWwindow* window, World& world) {
    Actors& actors = world.actors();
    select_cube(inputs,world);
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_PRESS,[&] () {std::cout << "You pressed escape\n"; });
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_REPEAT,[&] () {std::cout << "You held escape\n"; });

    inputs.setFunc(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, [&] () {
        const Id id = world.actors().selected();
        //std::cout << "Actor firing " << id << " " << world.actors().selectedActor().id << "\n";
        world.firedShot(id);
    });

    // must be capture by value here
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_RELEASE,[=] () {glfwSetWindowShouldClose(window, GLFW_TRUE); });

    inputs.setFunc1(GLFW_KEY_TAB,[&] () {
        actors.next(); 
        select_cube(inputs,world);        
    });
}

void select_cube(Window_Inputs& inputs, World& world) {
    Actors& actors = world.actors();
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
    inputs.setFunc2(GLFW_KEY_UP,[&] () {
            actors.apply_force(actors.selected(),Force(small*UP,Force::Type::Force));
    });
    inputs.setFunc2(GLFW_KEY_DOWN,[&] () {
            actors.apply_force(actors.selected(),Force(small*DOWN,Force::Type::Force));
    });
}
