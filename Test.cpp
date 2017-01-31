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
#include "Camera.hpp"
#include "G_Cuboid.hpp"
#include "data.hpp"

void gl_loop_start();

void set_keyboard(Window_Inputs& inputs, GLFWwindow* window);

static v3 camera_pos;
static fq camera_ori;
static v3 cube_pos;
static fq cube_ori;
float small = 0.02f;

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

    set_keyboard(inputs,window);

    G_Cuboid cube(&vertices,"shaders/vertex.shader","shaders/fragment.shader");

    while (!glfwWindowShouldClose(window)) {

        long newTime = timeNow();
        long frameTime = newTime - currentTime;
        currentTime = newTime;
        acc += frameTime;

        // process inputs, change world
        inputs.processInput(); // polls input and executes action based on that

        const v2 mouseDelta = inputs.cursorDelta();
        camera_ori = fq(0.05f * v3(glm::radians(-mouseDelta.y), glm::radians(-mouseDelta.x), 0.0f)) * camera_ori;
        //orient = fq(0.05f * v3(glm::radians(offset.y), glm::radians(offset.x), 0.0f)) * orient;

        // simulate world
        while (acc >= dt) {
            static const float normalize = 1.0f / (float)dt;
            const float t_normalized = t * normalize;
            const float dt_normalized = dt * normalize;

            // feeds in essentially a time value of 1 every time
            // since fixed time step
            acc -= dt;
            t += dt;
        }

        // Render -- -- --
        // local space -> world space -> view space -> clip space -> screen space
        //          model matrix   view matrix  projection matrix   viewport transform
        // Vclip = Mprojection * Mview * Mmodel * Vlocal
        //m4 model = me.modelMatrix();
        const v3 facing = camera_ori * FORWARD;
        const v3 up_relative = camera_ori * UP;
        m4 view = glm::lookAt(camera_pos, camera_pos + facing, up_relative);

        float aspectRatio = inputs.windowSize().x / inputs.windowSize().y;
        GLuint viewLoc = glGetUniformLocation(cube.shaderProgram(), "view");

        gl_loop_start();

        const G_Cuboid& graphical_cube = cube;
        graphical_cube.bindBuffers();
        graphical_cube.useShader();

        m4 projection = glm::perspective(glm::radians(90.0f), aspectRatio, 0.1f, 200.0f);
        GLuint projectionLoc = glGetUniformLocation(graphical_cube.shaderProgram(), "projection");

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(graphical_cube.VAO);

        m4 model;
        m4 rotation(glm::toMat4(cube_ori));
        //std::cout << "My orientation is " << printQ(orient) << "\n";
        //std::cout << "My euler " << printV(glm::eulerAngles(orient)) << "\n";
        m4 translation(glm::translate(m4(), cube_pos));
        model = translation * rotation;

        GLuint modelLoc = glGetUniformLocation(graphical_cube.shaderProgram(), "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, graphical_cube.drawSize());

        glBindVertexArray(0);
        glUseProgram(0);
        // Render end -- -- --

        // sleep if fps would be > fps_max
        long spareFrameTime = 1e6l / fps_max - (timeNow() - newTime);
        std::this_thread::sleep_for(std::chrono::microseconds(std::max(0l,spareFrameTime)));

        camera_ori = glm::normalize(camera_ori);

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

void set_keyboard(Window_Inputs& inputs, GLFWwindow* window) {
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_PRESS,[&] () {std::cout << "You pressed escape\n"; });
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_REPEAT,[&] () {std::cout << "You held escape\n"; });

    inputs.setFunc(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, [&] () {std::cout << "Left mouse\n"; });

    // must be capture by value here
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_RELEASE,[=] () {glfwSetWindowShouldClose(window, GLFW_TRUE); });

    // camera
    inputs.setFunc2(GLFW_KEY_W,[&] () {camera_pos+=small*(camera_ori*FORWARD); });
    inputs.setFunc2(GLFW_KEY_S,[&] () {camera_pos+=small*(camera_ori*BACKWARD); });
    inputs.setFunc2(GLFW_KEY_A,[&] () {camera_pos+=small*(camera_ori*LEFT); });
    inputs.setFunc2(GLFW_KEY_D,[&] () {camera_pos+=small*(camera_ori*RIGHT); });
    //inputs.setFunc2(GLFW_KEY_UP,[&] () {me.apply_force(UP); });
    //inputs.setFunc2(GLFW_KEY_DOWN,[&] () {me.apply_force(DOWN); });

    inputs.setFunc2(GLFW_KEY_R,[&] () {
        //cube_ori = cube_ori * LEFT;
        const v3 by = FORWARD * 0.02f;
        std::cout << "Cube ori b4 " << printQ(cube_ori) << " " << printV(by) << "\n";
        fq quat = fq(by);
        cube_ori = quat * cube_ori;
        std::cout << "New cube ori " << printQ(cube_ori) << "\n";
        cube_ori = glm::normalize(cube_ori);
    });
    inputs.setFunc2(GLFW_KEY_Y,[&] () {
        const v3 by = UP * 0.02f;
        std::cout << "Cube ori b4 " << printQ(cube_ori) << " " << printV(by) << "\n";
        fq quat = fq(by);
        cube_ori = quat * cube_ori;
        std::cout << "New cube ori " << printQ(cube_ori) << "\n";
        cube_ori = glm::normalize(cube_ori);
    });
    inputs.setFunc2(GLFW_KEY_Z,[&] () {
    });

    inputs.setFunc2(GLFW_KEY_UP,[&] () {
            cube_pos+=small*(cube_ori*FORWARD);
    });
    inputs.setFunc2(GLFW_KEY_DOWN,[&] () {
            cube_pos+=small*(cube_ori*BACKWARD);
    });
    inputs.setFunc2(GLFW_KEY_LEFT,[&] () {
            cube_pos+=small*(cube_ori*LEFT);
    });
    inputs.setFunc2(GLFW_KEY_RIGHT,[&] () {
            cube_pos+=small*(cube_ori*RIGHT);
    });
    
}
