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
#include <sstream>
#include <map>
#include <iterator>
#include <sstream>
#include <deque>
#include <memory>

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
#include "Shot.hpp"
#define ASIO_STANDALONE
#include "asio.hpp"

#include "Archiver.hpp"
#include "cereal/types/deque.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "Receiver.hpp"
#include "Sender.hpp"

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
 - Collision where they shot
 - Gravity
 - Perhaps camera?
 - Movement caching
*/

void gl_loop_start();
void set_keyboard(Window_Inputs& inputs, GLFWwindow* window, World& world);
void select_cube(Window_Inputs& inputs, World& world);
void draw_crosshair(Window_Inputs& inputs);

static const float my_mass = 1.0f;
static const float cube1_mass = 10.0f;
static const float cube2_mass = 2.0f;
static const float cube3_mass = 0.1f;
static const float default_mass = 1.0f;
static const float small = my_mass * 0.05f;
static const long program_start_time = timeNowMicros();
static bool server = false;
static bool client = false;

static unsigned short listen_port;
static std::vector<std::pair<std::string,std::string>> addresses; // address, port
static io_service io;

static std::unique_ptr<Receiver> receiver;
static std::vector<Sender> senders;

long static timeNow() {
    return timeNowMicros() - program_start_time;
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "Not enough arguments - please provide client/server as first param\n";
        exit(1);
    }

    // host, port
    std::string type(argv[1]);
    if (type == "server") {
        server = true;
        if (argc < 5) {
            std::cout << "To run server: ./server server [server_receive_port] [client_addr] [client_port]... - at least is needed\n";
            exit(1);
        }

        listen_port = std::stoi(argv[2]);
        for (int i=3; i<argc; i+=2) {
            std::string addr = argv[i];
            std::string port = argv[i+1];
            auto address = std::make_pair(addr,port);
            addresses.push_back(address);
        }
    } else if (type == "client") {
        client = true;
        if (argc < 5) {
            std::cout << "To run client: ./server client [client_receive_port] [server_host] [server_port] - at least is needed\n";
            exit(1);
        }
        listen_port = std::stoi(argv[2]);
        std::string addr = argv[3];
        std::string port = argv[4];
        auto address = std::make_pair(addr,port);
        addresses.push_back(address);
    }

    std::cout << "Listenport: " << listen_port << "\n";
    receiver = make_unique<Receiver>(io,listen_port);

    for (const auto& address: addresses) {
        // first:address, second:port
        std::cout << "Listeport: " << listen_port << " and address " << address.first << " and port " << address.second << "\n";
        senders.emplace_back(io, listen_port, address.first, address.second);
    }

    //_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    ////_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    Window_Inputs inputs;

    GLFWwindow* window = inputs.init_window(1280, 800);

    //const long fps_max = 60l;
    
    //const long tickrate = 100l;
    //const long dt = (1e6l)/tickrate; // run at tickrate 100

    long t = 0l;
    long lastFrameStart = timeNow();
    long acc = 0l;

    const float areaSize = 500.0f;

    const float restitution = 0.05f;

    World world(areaSize, inputs.windowSize(), restitution);
    
    // make default cube with index in map 0
    int default_g_cube = 0;

    world.g_cubs.emplace(
            std::make_pair(default_g_cube,
            G_Cuboid(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader"))
    );

    /*
    Actor(
    int g_cub, // index to display cuboid in the world.g_cubs map (int->G_Cuboid)
    const fv* vertexData, // used to construct logical cuboid - note could change
    // so that logical cuboid completely different to graphical data
    v3 scale, // sizing, ie. to make cuboid (2.0,1.0,2.0)
    v3 startPos, // spawn point, obv. important for non-mobile
    float mass,
    float inertia,
    bool selectable, // can tab to it or not
    bool mobile // can move etc.
    );
    */

    auto create_default_cube = [&] (v3 start_pos, float mass, bool selectable, bool mobile) {
        world.insert(
                new Actor(default_g_cube,
                    &vertices,
                    v3(1.0f,1.0f,1.0f),
                    start_pos,
                    mass,
                    5.0f,
                    selectable,
                    mobile)
                );
    };

    auto spawn_cubes = [&] (v3 where, int n, int m, bool rotated, bool selectable, bool mobile) {
        static const float seperator = 1.0f;
        for (int i=0; i<n; ++i) {
            for (int j=0; j<m; ++j) {
                const float ang_sep = 1.5f;
                v3 position(ang_sep*(seperator*(float)i-n/2), 0.0f, ang_sep*(seperator*(float)j-m/2));
                position += where;
                create_default_cube(position,default_mass,selectable,mobile);
                if (rotated) {
                    world.apply_force(Force(std::max(world.actors().size()-1,0),v3(i,0.0f,j),Force::Type::Torque));
                }
            }
        }
    };

    create_default_cube(v3(0.0f,-0.4f,10.0f),my_mass,true,true);

    world.insert(
            new Actor(default_g_cube,
                &vertices,
                v3(4.0f,1.0f,4.0f),
                v3(15.0f,2.0f,0.0f),
                cube1_mass,
                5.0f,
                true,
                true)
            );
    create_default_cube(v3(20.0f,2.0f,3.0f),cube2_mass,true,true);
    create_default_cube(v3(-20.0f,2.0f,3.0f),cube3_mass,true,true);

    spawn_cubes(v3(0.0f,-1.0f,0.0f),25,10,true,false,true);
    spawn_cubes(v3(0.0f,-10.0f,0.0f),25,10,false,false,true);

    set_keyboard(inputs,window,world);

    static int frame = 0;

    //float fps = 60.0f;
    //float f_time = 1e6f / fps;
    const double rate = 80.0;
    const long dt = (1e6l)/100.0; // run at tickrate 100
    double frame_time = 1e6 / rate;

    long temp = timeNow();

    // simple as tits loop with render/tick bound for now - just easier to work with
    while (!glfwWindowShouldClose(window)) {
        ++frame;
        long frameStart = timeNow();

            // process inputs, change world
            inputs.processInput(); // polls input and executes action based on that

            const v2 mouseDelta = inputs.cursorDelta();
            //bool gf = inputs.window_gained_focus();
            //bool lf = inputs.window_lost_focus();
            // these functions reset a bool, should be called every frame really
            const v3 mouse_torque = v3(glm::radians(mouseDelta.y), glm::radians(mouseDelta.x), 0.0f);
            world.apply_force(Force(world.actors().selected(),mouse_torque,Force::Type::Torque,false,false));

            static const float normalize = 1.0f / 1e4f;
            const float t_normalized = t * normalize;
            const float dt_normalized = dt * normalize;

            temp = timeNow();
            world.simulate(t_normalized,dt_normalized);
            //std::cout << "Time for sim " << (double)(timeNow()-temp)/1000.0 << "ms\n";

            temp = timeNow();
            world.collisions();
            //std::cout << "Time for col " << (double)(timeNow()-temp)/1000.0 << "ms\n";

            const Forces& forces = world.forces();
            world.apply_forces(forces);
            // actually applies forces and moves the world
            
            const Shots& shots = world.shots();
            world.fire_shots(shots);
            // actually fires the shots

        // Render -- -- --
        // local space -> world space -> view space -> clip space -> screen space
        //          model matrix   view matrix  projection matrix   viewport transform
        gl_loop_start();

        //temp = timeNow();
        world.render(); // render renders current state - so if rendering shots, must clear after rendering
        draw_crosshair(inputs);
        //std::cout << "Time for render " << (double)(timeNow()-temp)/1000.0 << "ms\n";
        inputs.swapBuffers(); // swaps buffers

            world.clear_forces();
            world.clear_shots();

        // sleep if fps would be > fps_max
        //std::this_thread::sleep_for(std::chrono::microseconds(std::max(0l,spareFrameTime)));

        long thisFrameTime_u = timeNow() - frameStart;
        //std::cout << "This frame " << thisFrameTime_u << "u \n";
        //std::cout << "Max pos " << frame_time << "u \n";
        long spare_time_u = (long)frame_time - thisFrameTime_u;
        //std::cout << "Spare time " << (double)spare_time_u/1000.0 << "ms" << "\n";
        std::this_thread::sleep_for(std::chrono::microseconds(std::max(0l,spare_time_u)));
    }

    inputs.close();

}

    /*
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
            //bool gf = inputs.window_gained_focus();
            //bool lf = inputs.window_lost_focus();
            // these functions reset a bool, should be called every frame really
            const v3 mouse_torque = v3(glm::radians(mouseDelta.y), glm::radians(mouseDelta.x), 0.0f);
            world.apply_force(world.actors().selected(),Force(mouse_torque,Force::Type::Torque,false,false));

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
    */

void gl_loop_start() {
    // Clear the colorbufer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void set_keyboard(Window_Inputs& inputs, GLFWwindow* window, World& world) {
    select_cube(inputs,world);
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_PRESS,[&] () {std::cout << "You pressed escape\n"; });
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_REPEAT,[&] () {std::cout << "You held escape\n"; });

    inputs.setFunc(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, [&] () {
        const Id id = world.actors().selected();
        //std::cout << "Actor firing " << id << " " << world.actors().selectedActor().id << "\n";
        world.fire_shot(id);
    });

    // must be capture by value here
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_RELEASE,[=] () {glfwSetWindowShouldClose(window, GLFW_TRUE); });

    inputs.setFunc1(GLFW_KEY_TAB,[&] () {
        world.actors().next(); 
        select_cube(inputs,world);        
    });
}

void select_cube(Window_Inputs& inputs, World& world) {
    Actors& actors = world.actors();
    // camera
    inputs.setFunc1(GLFW_KEY_P,[&] () {
        inputs.toggle_cursor();
    });
    inputs.setFunc1(GLFW_KEY_SPACE,[&] () {
        world.blow_up(world.actors().selected());
    });

    inputs.setFunc2(GLFW_KEY_R,[&] () {
            world.apply_force(Force(world.actors().selected(),LEFT,Force::Type::Torque,false,true));
    });
    inputs.setFunc2(GLFW_KEY_Y,[&] () {
            world.apply_force(Force(world.actors().selected(),UP,Force::Type::Torque,false,true));
    });
    inputs.setFunc2(GLFW_KEY_Z,[&] () {
            world.apply_force(Force(world.actors().selected(),FORWARD,Force::Type::Torque,false,true));
    });

    inputs.setFunc2(GLFW_KEY_W,[&] () {
            world.apply_force(Force(world.actors().selected(),small*FORWARD,Force::Type::Force));
    });
    inputs.setFunc2(GLFW_KEY_S,[&] () {
            world.apply_force(Force(world.actors().selected(),small*BACKWARD,Force::Type::Force));
    });
    inputs.setFunc2(GLFW_KEY_A,[&] () {
            world.apply_force(Force(world.actors().selected(),small*LEFT,Force::Type::Force));
    });
    inputs.setFunc2(GLFW_KEY_D,[&] () {
            world.apply_force(Force(world.actors().selected(),small*RIGHT,Force::Type::Force));
    });
    inputs.setFunc2(GLFW_KEY_UP,[&] () {
            world.apply_force(Force(world.actors().selected(),small*UP,Force::Type::Force));
    });
    inputs.setFunc2(GLFW_KEY_DOWN,[&] () {
            world.apply_force(Force(world.actors().selected(),small*DOWN,Force::Type::Force));
    });
}

void draw_crosshair(Window_Inputs& inputs) {
    // 10 minute crosshair
    // An array of 3 vectors which represents 3 vertices
    v2 normed = glm::normalize(inputs.windowSize());
    float rat = normed.y/normed.x;
    static GLfloat g_vertex_buffer_data[] = {
        -0.5f*rat, 0.0f, 0.0f,
        0.5f*rat, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f,
        0.0f, -0.5f, 0.0f
    };

    bool once = false;
    // This will identify our vertex buffer
    static GLuint vertexbuffer;
    if (!once) {
        // Generate 1 buffer, put the resulting identifier in vertexbuffer
        glGenBuffers(1, &vertexbuffer);
        once = true;
    }
    // The following commands will talk about our 'vertexbuffer' buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    // Give our vertices to OpenGL.
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    // 1st attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(
            0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
            );
    // Draw the triangle !
    glDrawArrays(GL_LINES, 0, 4); // Starting from vertex 0; 3 vertices total -> 1 triangles
    glDisableVertexAttribArray(0);
}
