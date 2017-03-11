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

static const float my_mass = 1.0f;
static const float cube1_mass = 20.0f;
static const float cube2_mass = 2.0f;
static const float cube3_mass = 0.1f;
static const float default_mass = 1.0f;
static const float small = my_mass * 0.05f;
static const long program_start_time = timeNowMicros();

static std::string type;
static const std::string type_server = "server";
static const std::string type_client = "client";
static const std::string type_local = "local"; // no networking

static unsigned short local_port; // port that the socket_ptr binds to
// ie port that stuff gets sent from and read from
static std::shared_ptr<udp_socket> socket_ptr;
static std::vector<std::pair<std::string,std::string>> addresses; // address, port

static io_service io;
static std::unique_ptr<Receiver> receiver_ptr;
static std::vector<Sender> senders;
static std::string my_id;

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

    if (argc < 2) {
        std::cout << "Not enough arguments - please provide client/server as first param\n";
        exit(1);
    }

    // host, port
    type = std::string(argv[1]);
    if (type == type_server) {
        if (argc < 5 || argc % 2 == 0) {
            std::cout << "To run server: ./server server [server_receive_port] [client_addr] [client_port]... - at least is needed\n";
            exit(1);
        }

        local_port = std::stoi(argv[2]);
        for (int i=3; i<argc; i+=2) {
            std::string addr = argv[i];
            std::string port = argv[i+1];
            auto address = std::make_pair(addr,port);
            addresses.push_back(address);
        }
    } else if (type == type_client) {
        if (argc != 5) {
            std::cout << "To run client: ./client client [client_receive_port] [server_host] [server_port] at least is needed\n";
            exit(1);
        }
        local_port = std::stoi(argv[2]);
        std::string addr = argv[3];
        std::string port = argv[4];
        auto address = std::make_pair(addr,port);
        addresses.push_back(address);
        my_id = local_port;
    } else if (type == type_local) {
    } else {
        std::cout << "Did not recognise type, choose from either " << type_server << " or " << type_client << "\n";
        exit(1);
    }

    socket_ptr = std::make_shared<udp_socket>(io, udp_endpoint(asio::ip::udp::v4(), local_port));

    std::cout << "Local port: " << local_port << "\n";
    receiver_ptr = make_unique<Receiver>(io,socket_ptr);

    for (const auto& address: addresses) {
        // first:address, second:port
        std::cout << "Local port: " << local_port << " and address " << address.first << " and port " << address.second << "\n";
        senders.emplace_back(io, socket_ptr, address.first, address.second);
    }

    //_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    ////_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    Window_Inputs inputs;

    GLFWwindow* window = inputs.init_window(type, 640, 480);
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

    static int frame = 0;

    const double rate = 80.0; // tickrate/framerate (bound currently)
    const long dt = (1e6l)/100.0; // just don't change this for god's sake
    const double frame_time = 1e6 / rate;

    long temp = timeNow();

    // apply initial setup forces on server

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

            temp = timeNow();
            world.collisions();
            //std::cout << "Time for col " << (double)(timeNow()-temp)/1000.0 << "ms\n";

            if (type == type_client) {

                // process forces and send to server
                Forces& forces = world.forces();
                if (forces.size() > 0) {
                    auto serial = Sender::serialize(forces);
                    assert(senders.size() == 1);
                    for (auto& sender: senders) {
                        //std::cout << "Client sending force\n";
                        sender.send(serial);
                    }
                }

                // if server has sent reply, apply that to world
                while (receiver_ptr->available()) {
                    //std::cout << "Client receiving force\n";
                    Forces fs;
                    fs = receiver_ptr->receive<Forces>();
                    world.apply_forces(fs);
                }

            } else if (type == type_server) {

                // if clients have sent any forces process them and affect world state
                // then send them to all clients - currently doing this immediately
                Forces forces;
                while (receiver_ptr->available()) {
                    //std::cout << "Server receiving force\n";
                    Forces forces_in = receiver_ptr->receive<Forces>();
                    forces.insert(forces.begin(), forces_in.begin(), forces_in.end());
                }
                // in real thing obv send update every frame!
                if (forces.size() > 0) {
                    //std::cout << "Server sending force\n";
                    // apply force to world
                    world.apply_forces(forces);
                    // send forces to clients
                    auto serial = Sender::serialize(forces);
                    for (auto& sender: senders) {
                        sender.send(serial);
                    }
                }
            } else if (type == type_local) {
                // assumed testing non networking aspect
                // shots must be first for recoil force
                world.fire_shots(world.shots());
                world.apply_forces(world.forces());
            }

            // actually applies forces and moves the world

            //Shots& shots = world.shots();
            //world.fire_shots(shots);
            // actually fires the shots

            static const float normalize = 1.0f / 1e4f;
            const float t_normalized = t * normalize;
            const float dt_normalized = dt * normalize;

            temp = timeNow();
            world.simulate(t_normalized,dt_normalized);
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
        renderText(text_shader, "420 blaze it", 25.0f, 25.0f, 1.0f, glm::vec3(1.0f, 0.8f, 1.0f));

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
    int default_g_cube = 0;
    int colored_g_cube = 1;
    int default_l_face_verts = 0;

    world.l_cub_face_verts.emplace(
            std::make_pair(default_l_face_verts,
                face_verts_from_fv(&vertices))
    );

    world.g_cubs.emplace(
            std::make_pair(default_g_cube,
            make_unique<G_Cuboid>(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader"))
    );

    world.g_cubs.emplace(
            std::make_pair(colored_g_cube,
            make_unique<G_Cuboid_Color>(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader",&colors))
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
                Actor(default_g_cube,
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

    create_default_cube(v3(0.0f,-0.4f,10.0f),my_mass,true,true);

    world.insert(
            Actor(colored_g_cube,
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
    spawn_cubes(v3(0.0f,-10.0f,0.0f),25,10,false,false,true);
    spawn_cubes(v3(0.0f,-20.0f,0.0f),25,10,false,false,true);
    spawn_cubes(v3(0.0f,-30.0f,0.0f),25,10,false,false,true);
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
