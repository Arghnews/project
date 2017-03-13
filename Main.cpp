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

#include <ft2build.h>
#include FT_FREETYPE_H

#include "Archiver.hpp"
#include "Receiver.hpp"
#include "Sender.hpp"

#include "Packet_Header.hpp"
#include "Packet_Payload.hpp"
#include "Packet.hpp"
#include "Connection.hpp"
#include "Deltas.hpp"

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
Forces setup_cubes(World& world);
void draw_crosshair(Window_Inputs& inputs);
void static parse_args(
        int argc,
        char* argv[],
        std::string& instance_type,
        Instance_Id& instance_id,
        Connection_Addresses& connection_addresses
        );

static const float my_mass = 1.0f;
static const float cube1_mass = 20.0f;
static const float cube2_mass = 2.0f;
static const float cube3_mass = 0.1f;
static const float default_mass = 1.0f;
static const float small = my_mass * 0.05f;
static const long program_start_time = timeNowMicros();

static const std::string type_server = "server";
static const std::string type_client = "client";
static const std::string type_local = "local"; // no networking

static std::string instance_type; // server/client etc.
static Instance_Id instance_id; // 0-255, server usually 0

static int received_seqs_lim = 40;

static uint32_t tick = 0;

//
// 
//
// text related stuff
// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    GLuint TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
    GLuint Advance;    // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;
GLuint VAO_text, VBO_text;
//Shader text_shader("shaders/text.vertex.shader", "shaders/text.fragment.shader");

void setup_text();
void bind_text_graphics(Shader& text_shader, float window_width, float window_height);
void renderText(Shader &shader, std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color);

// end of text related stuff
//
//
//

long static timeNow() {
    return timeNowMicros() - program_start_time;
}

int main(int argc, char* argv[]) {

    io_service io;
    Connections connections;
    Connection_Addresses connection_addresses;


    parse_args(argc, argv,
            instance_type,
            instance_id,
            connection_addresses);

    //_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    ////_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);


    // networking
    // if want printouts, uncomment the //// lines in Connection.cpp

    // setup connections
    if (instance_type == type_server) {
        for (int i=0; i<connection_addresses.size(); ++i) {
            if (i == instance_id) {
                continue;
                // don't want server connecting to self
            }
            const auto& connection_address = connection_addresses[i];
            //std::cout << "Making connection to " << connection_address.remote_port << "\n";
            connections.emplace_back(io,
                    connection_address,
                    instance_id, received_seqs_lim);
            //std::cout << "Success\n";
        }
    } else if (instance_type == type_client) {
        // add server with ports swapped and remote_host as server address
        connections.emplace_back(io,
                Connection_Address::clientify(connection_addresses, instance_id),
                instance_id, received_seqs_lim);
    }

    Window_Inputs inputs;

    std::string win_name = instance_type + " " + std::to_string(int(instance_id));
    GLFWwindow* window = inputs.init_window(win_name, 640, 480);
    //const long fps_max = 60l;
    
    //const long tickrate = 100l;
    //const long dt = (1e6l)/tickrate; // run at tickrate 100

    Shader text_shader("shaders/text.vertex.shader", "shaders/text.fragment.shader");
    setup_text();

    long t = 0l;
    long lastFrameStart = timeNow();
    long acc = 0l;

    const float areaSize = 500.0f;
    World world(areaSize, inputs.windowSize());

    Forces forces = setup_cubes(world);
    world.apply_forces(forces);
    forces.clear();

    set_keyboard(inputs,window,world);

    auto selected_id = std::max((uint8_t)0,instance_id);
    world.actors().select(selected_id);

    static int frame = 0;

    const double rate = 80.0; // tickrate/framerate (bound currently)
    const long dt = (1e6l)/100.0; // just don't change this for god's sake
    const double frame_time = 1e6 / rate;

    long temp = timeNow();

    // apply initial setup forces on server
    
    std::string render_text = "";

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
            if (!(mouse_torque.x == 0.0f && mouse_torque.y == 0.0f)) {
                world.apply_force(Force(world.actors().selected(),mouse_torque,Force::Type::Torque,false,false));
            }

            static const float normalize = 1.0f / 1e4f;
            const float t_normalized = t * normalize;
            const float dt_normalized = dt * normalize;

            temp = timeNow();
            //std::cout << "Time for col " << (double)(timeNow()-temp)/1000.0 << "ms\n";

            if (instance_type == type_client) {

                Connection& conn = connections[0];

                assert(connections.size() == 1 && "Client should only have one connection (to server)");
                Packet_Payload payload(Packet_Payload::Type::Input, tick);
                payload.forces = world.forces();
                payload.shots = world.shots();
                conn.send(payload);
                //p = gen_payload();
                //conn.send(p);
                Packet_Payloads payloads = conn.receive();
                //std::cout << int(conn.instance_id()) << " received ticks ("<<payloads.size() <<")\n";
                // CHANGE ME ---------------------------------------------------------------------------------------------------------------------------------------------------
                for (const auto& received_payload: payloads) {
                    //std::cout << "Recevied payload for tick " << received_payload.tick << "\n";
                    assert(received_payload.type == Packet_Payload::Type::State && "Client should only receive stateful packets from server");
                    for (const auto& mom_pos: received_payload.vec_mom_pos) {
                        const Id& id = mom_pos.id;
                        Actor& actor = world.actors()[id];
                        P_State& p_state = actor.state_to_change();
                        p_state.position = mom_pos.position;
                        p_state.momentum = mom_pos.momentum;
                        // world
                        // set actor mom_pos.id to have mom mom_pos.momentum and pos..
                    }
                    for (const auto& angmom_orient: received_payload.vec_angmom_ori) {
                        const Id& id = angmom_orient.id;
                        Actor& actor = world.actors()[id];
                        P_State& p_state = actor.state_to_change();
                        p_state.ang_momentum = angmom_orient.ang_momentum;
                        p_state.orient = angmom_orient.orient;
                        // world
                        // set actor mom_pos.id to have mom mom_pos.momentum and pos..
                    }
                    const Id& id = world.actors().selected();
                    for (const auto& shot: received_payload.shots) {
                        if (shot.shooter == id) {
                            // you shot someone
                            render_text = "You shot";
                            if (shot.hit) {
                                render_text += " " + std::to_string(shot.target);
                            } else {
                                render_text += " and missed";
                            }
                        } else if (shot.target == id && shot.hit) {
                            render_text = "You were shot by " + std::to_string(shot.shooter);
                        }
                    }
                }
                world.clear_forces();
                world.clear_shots();
                    // add this whole payload for whatever tick this was to buffer
                    // buffer should be ordered by tick
                    /* // Potentially in received_payload, need to set state to these/add to buffer!
    Tick tick; // initially tied to Seq number
    // application level number, so can tell at what tick this was sent on
    // can tell if too old or not etc // think I need this? maybe not really
    Type type;
    Forces forces;
    Shots shots;
    // defined in Deltas.hpp
    std::vector<Mom_Pos> vec_mom_pos;
    std::vector<AngMom_Ori> vec_angmom_ori;
                       */
                    //std::cout << "Tick:" << payload.tick << "\n";
            } else if (instance_type == type_server) {
                /*
                   Process inputs from all clients
                   for (const auto& payload: payloads) {
                   world.apply_forces(payload.forces); // etc
                   }
                // for every shot in payload.shots
                // add shots to world.fire_shot(shot); // etc
                */
                // not sure about order of these lines perhaps
                // apply forces from received input
                
                // for every connection (client)
                //   while there are packets to read
                //     for every payload
                //       apply all the forces in the payload to the world
                for (auto& conn: connections) {
                    while (conn.available()) {
                        Packet_Payloads payloads = conn.receive();
                        //std::cout << int(conn.instance_id()) << " received ticks ("<<payloads.size() <<")\n";
                        for (const auto& payload: payloads) {
                            for (const auto& force: payload.forces) {
                                world.apply_force(force);
                            }
                            for (const auto& shot: payload.shots) {
                                world.fire_shot(shot);
                            }
                        }
                    }
                }

                world.collisions();
                std::vector<Mom_Pos> vec_mom_pos;
                vec_mom_pos.reserve(2);
                std::vector<AngMom_Ori> vec_angmom_ori;


                Packet_Payload payload(Packet_Payload::Type::State, tick);
                payload.shots = world.fire_shots(world.shots());

                world.apply_forces(world.forces());
                world.simulate(t_normalized,dt_normalized,
                        vec_mom_pos, vec_angmom_ori);

                payload.vec_mom_pos = vec_mom_pos;
                payload.vec_angmom_ori = vec_angmom_ori;

                for (auto& connection: connections) {
                    connection.send(payload);
                }

                world.clear_forces();
                world.clear_shots();

                // world.fire_shots(world.shots())
                // need to get from simulate, any actors states that changed
                //std::vector<Id_v3> momentums = world.new_momentums(); // or something
                // world.clear_new_momentums();
                // 
                // push to all clients new world state
                // for (auto& conn: connections) {
                // Packet_Payload payload(Packet_Payload::Type::State, tick);
                // payload.momentums = new_momentums;
                // conn.send(payload);
                // 
                // world.clear_forces();
                // world.clear_shots();
                // perhaps need to add something stateful about like
                // pair of just been shot here so that client can tell
                /*
                // remember something about some issue about how shots must not be cleared
                // till after render? -- if strange shot behaviour
                // if clients have sent any forces process them and affect world state
                // then send them to all clients - currently doing this immediately
                }*/
            } else if (instance_type == type_local) {
                // assumed testing non networking aspect
                // shots must be first for recoil force
                world.collisions();
                world.fire_shots(world.shots());
                world.apply_forces(world.forces());
                std::vector<Mom_Pos> vec_mom_pos;
                std::vector<AngMom_Ori> vec_angmom_ori;
                world.simulate(t_normalized,dt_normalized,
                        vec_mom_pos, vec_angmom_ori);
            }

            // actually applies forces and moves the world

            //Shots& shots = world.shots();
            //world.fire_shots(shots);
            // actually fires the shots

            temp = timeNow();
            //std::cout << "Time for sim " << (double)(timeNow()-temp)/1000.0 << "ms\n";


        // Render -- -- --
        // local space -> world space -> view space -> clip space -> screen space
        //          model matrix   view matrix  projection matrix   viewport transform
        gl_loop_start();

        temp = timeNow();
        world.render(); // render renders current state -
        // -- NOTE -- so if rendering shots, must clear after rendering
        draw_crosshair(inputs);

        v2 win = inputs.windowSize();
        bind_text_graphics(text_shader,win.x,win.y);
        renderText(text_shader, render_text, 10.0f, 10.0f, 1.0f, glm::vec3(1.0f, 0.8f, 1.0f));

        ++tick;

        inputs.swapBuffers(); // swaps buffers
        //std::cout << "Time for render " << (double)(timeNow()-temp)/1000.0 << "ms\n";

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
    for (auto& conn: connections) {
        while (conn.available()) {
            conn.receive();
        }
    }
    for (auto& conn: connections) {
        while (conn.available()) {
            conn.close();
        }
    }
    io.stop();
    std::cout << int(instance_id) << " shutting down\n";
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

// turns 108 GLfloat data floats into 24 glm::vec3s that correspond to faces
// eg. 4 verts per face, 6 faces
vv3 face_verts_from_fv(const fv* points_in) {
    const fv& points = *points_in;
    // first calc the faces
    const int size = points.size(); // 3d
    // 108 points -> faces
    vv3 faces; // 24 vertices
    assert(size == 108);
    for (int i=0; i<size; i+=18) {
        vv3 square;
        square.reserve(6);
        square.push_back(v3(points[i+0], points[i+1], points[i+2]));
        square.push_back(v3(points[i+3], points[i+4], points[i+5]));
        square.push_back(v3(points[i+6], points[i+7], points[i+8]));
        square.push_back(v3(points[i+9], points[i+10], points[i+11]));
        square.push_back(v3(points[i+12], points[i+13], points[i+14]));
        square.push_back(v3(points[i+15], points[i+16], points[i+17]));
        square = unique(square);
        concat(faces, square);
    }
    assert(faces.size() == 24);

    // all the unique points in the faces are the verts, size 8
    //originalVertices_ = faces;
    return faces;
}

Forces setup_cubes(World& world) {
    Forces forces;
    // make default cube with index in map 0
    int g_cub_default_colors = 0;
    int g_cub_colors_blue = 1;
    int g_cub_colors_green = 2;
    int g_cub_colors_red = 3;
    int default_l_face_verts = 0;

    world.l_cub_face_verts.emplace(
            std::make_pair(default_l_face_verts,
                face_verts_from_fv(&vertices))
    );

    world.g_cubs.emplace(
            std::make_pair(g_cub_default_colors,
            make_unique<G_Cuboid>(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader"))
    );

    world.g_cubs.emplace(
            std::make_pair(g_cub_colors_red,
            make_unique<G_Cuboid_Color>(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader",&colors_red))
    );

    world.g_cubs.emplace(
            std::make_pair(g_cub_colors_blue,
            make_unique<G_Cuboid_Color>(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader",&colors_blue))
    );

    world.g_cubs.emplace(
            std::make_pair(g_cub_colors_green,
            make_unique<G_Cuboid_Color>(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader",&colors_green))
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
                Actor(g_cub_default_colors,
                    &world.l_cub_face_verts[default_l_face_verts],//&vertices,
                    v3(1.0f,1.0f,1.0f),
                    start_pos,
                    mass,
                    2.5f,
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
                    forces.emplace_back(std::max(world.actors().size()-1,0),v3(i*100.0f,0.0f,j*100.0f),Force::Type::Torque);
                }
            }
        }
    };

    world.insert(
            Actor(g_cub_colors_red,
                &world.l_cub_face_verts[default_l_face_verts],//&vertices,
                v3(1.0f),
                v3(0.0f,1.0f,14.0f),
                1.0f,
                3.0f,
                true,
                true)
            );

    world.insert(
            Actor(g_cub_colors_blue,
                &world.l_cub_face_verts[default_l_face_verts],//&vertices,
                v3(1.0f),
                v3(0.0f,-0.4f,10.0f),
                my_mass,
                3.0f,
                true,
                true)
            );

    world.insert(
            Actor(g_cub_colors_green,
                &world.l_cub_face_verts[default_l_face_verts],//&vertices,
                v3(4.0f,1.0f,4.0f),
                v3(15.0f,2.0f,0.0f),
                cube1_mass,
                10.0f,
                true,
                true)
            );
    create_default_cube(v3(20.0f,2.0f,3.0f),cube2_mass,true,true);
    create_default_cube(v3(-20.0f,2.0f,3.0f),cube3_mass,true,true);

    spawn_cubes(v3(0.0f,-1.0f,0.0f),25,10,true,false,true);
    spawn_cubes(v3(0.0f,-10.0f,0.0f),5,10,false,false,true);
    spawn_cubes(v3(0.0f,-20.0f,0.0f),5,10,false,false,true);
    spawn_cubes(v3(0.0f,-30.0f,0.0f),2,10,false,false,true);
    return forces;
}

void setup_text() {
    // FreeType
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

    // Load font as face
    FT_Face face;
    if (FT_New_Face(ft, "fonts/Blazed.ttf", 0, &face))
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

    // Load first 128 characters of ASCII set
    for (GLubyte c = 0; c < 128; c++)
    {
        // Load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void bind_text_graphics(Shader& text_shader, float window_width, float window_height) {

    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(window_width), 0.0f, static_cast<GLfloat>(window_height));
    text_shader.use();
    glUniformMatrix4fv(glGetUniformLocation(text_shader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Set OpenGL options
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    static bool once = false;
    if (!once) {
        once = true;
        // Configure VAO/VBO for texture quads
        glGenVertexArrays(1, &VAO_text);
        glGenBuffers(1, &VBO_text);
    }
    glBindVertexArray(VAO_text);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_text);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

}


void renderText(Shader &shader, std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color) {
    // Activate corresponding render state	
    shader.use();
    glUniform3f(glGetUniformLocation(shader.Program, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO_text);

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) 
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;
        // Update VBO for each character
        GLfloat char_vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },            
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }           
        };
        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO_text);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(char_vertices), char_vertices); // Be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void static parse_args(
        int argc,
        char* argv[],
        std::string& instance_type,
        Instance_Id& instance_id,
        Connection_Addresses& connection_addresses
        ) {
    if (argc == 2) {
        if (std::string(argv[1]) == type_local) {
            instance_type = type_local;
            return;
        }
    }
    if (argc < 9) {
        std::cerr << "Args of form: ./exec [client|server] id (local_port remote_host remote_port){2,}\n";
        std::cerr << "instance_type is " << type_server << "," << type_client;
        std::cerr << " or " << type_local << " and id 0-255" << "\n";
        exit(1);
    }

    instance_type = std::string(argv[1]);

    if (instance_type != type_server &&
            instance_type != type_client &&
            instance_type != type_local) {
        std::cerr << "instance_type is " << type_server << "," << type_client;
        std::cerr << " or " << type_local << "\n";
        exit(1);
    }

    if (instance_type == type_local) {
    } else {

        int potential_id = std::stoi(argv[2]);
        if (potential_id < 0 || potential_id > 255) {
            std::cerr << "Id must be from 0 to num instances-1 and must be in byte range (it's an array index\n";
            exit(1);
        }
        instance_id = potential_id;

        int start_of_rest = 3;
        if ((argc-start_of_rest) % 3 != 0 || (argc-start_of_rest) < 6) {
            std::cerr << "Arguments at end should [local_port remote_host remote_port] triplets\n";
            exit(1);
        }
        for (int i=start_of_rest; i<argc; i+=3) {
            std::string local_port = argv[i+0];
            std::string remote_host = argv[i+1];
            std::string remote_port = argv[i+2];
            connection_addresses.emplace_back(
                    Connection_Address(local_port, remote_host, remote_port));
        }

    }
    std::cout << "\n";
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
