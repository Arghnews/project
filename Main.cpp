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

#include "Util.hpp"
#include "Shader.hpp"
#include "Window_Inputs.hpp"
#include "Camera.hpp"
#include "G_Cuboid.hpp"
#include "data.hpp"
#include "Actor.hpp"
#include "Physics.hpp"

void gl_loop_start();
void set_keyboard(Window_Inputs& inputs, GLFWwindow* window, Actor&);
void set_keyboard(Window_Inputs& inputs, GLFWwindow* window, Actor& me, Actor& cube);

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

    Actor me(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f),
            10.0f, 10.0f);

    Actor cube(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f),
            10.0f, 10.0f);

    set_keyboard(inputs,window,me,cube);

    Physics phys;

    /*
    const L_Cuboid& my_cub = me.logical_cuboid();
    Actor them(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f),
            10.0f);
    phys.integrate(them.state_to_change(), t, dt);
    const L_Cuboid& their_cub = them.logical_cuboid();
    bool areColliding = L_Cuboid::colliding(my_cub, their_cub);
    if (areColliding) {
        // resolve
        std::cout << "Colliding\n";
    }
    */

    while (!glfwWindowShouldClose(window)) {

        long newTime = timeNow();
        long frameTime = newTime - currentTime;
        currentTime = newTime;
        acc += frameTime;

        // process inputs, change world
        inputs.processInput(); // polls input and executes action based on that

        const v2 mouseDelta = inputs.cursorDelta();
        //camera.rotate(mouseDelta);

        // simulate world
        while (acc >= dt) {
            //integrate(state, t, dt);
            P_State& my_phys = me.state_to_change();
            P_State& cube_phys = cube.state_to_change();
            // feeds in essentially a time value of 1 every time
            // since fixed time step
            const float normalize = 1.0f / (float)dt;
            const float t_normalized = t * normalize;
            const float dt_normalized = dt * normalize;
            phys.integrate(my_phys, t_normalized, dt_normalized);
            phys.integrate(cube_phys, t_normalized, dt_normalized);
            acc -= dt;
            t += dt;
        }

        // Render -- -- --
        // local space -> world space -> view space -> clip space -> screen space
        //          model matrix   view matrix  projection matrix   viewport transform
        // Vclip = Mprojection * Mview * Mmodel * Vlocal
        //m4 model = me.modelMatrix();
        m4 model = cube.modelMatrix();
        m4 view = me.viewMatrix();

        gl_loop_start();

        const G_Cuboid& graphical_cube = cube.graphical_cuboid();

        graphical_cube.bindBuffers();
        graphical_cube.useShader();

        float aspectRatio = inputs.windowSize().x / inputs.windowSize().y;
        m4 projection = glm::perspective(glm::radians(90.0f), aspectRatio, 0.1f, 200.0f);
        GLuint viewLoc = glGetUniformLocation(graphical_cube.shaderProgram(), "view");
        GLuint projectionLoc = glGetUniformLocation(graphical_cube.shaderProgram(), "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(graphical_cube.VAO);

        GLuint modelLoc = glGetUniformLocation(graphical_cube.shaderProgram(), "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, graphical_cube.drawSize());

        glBindVertexArray(0);
        glUseProgram(0);
        // Render end -- -- --
        
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

void set_keyboard(Window_Inputs& inputs, GLFWwindow* window, Actor& me, Actor& cube) {
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_PRESS,[&] () {std::cout << "You pressed escape\n"; });
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_REPEAT,[&] () {std::cout << "You held escape\n"; });

    inputs.setFunc(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, [&] () {std::cout << "Left mouse\n"; });

    // must be capture by value here
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_RELEASE,[=] () {glfwSetWindowShouldClose(window, GLFW_TRUE); });

    // camera
    inputs.setFunc2(GLFW_KEY_W,[&] () {me.apply_force(FORWARD); });
    inputs.setFunc2(GLFW_KEY_S,[&] () {me.apply_force(BACKWARD); });
    inputs.setFunc2(GLFW_KEY_A,[&] () {me.apply_force(LEFT); });
    inputs.setFunc2(GLFW_KEY_D,[&] () {me.apply_force(RIGHT); });
    inputs.setFunc2(GLFW_KEY_UP,[&] () {me.apply_force(UP); });
    inputs.setFunc2(GLFW_KEY_DOWN,[&] () {me.apply_force(DOWN); });

    inputs.setFunc1(GLFW_KEY_R,[&] () {cube.apply_torque(v3(0.0f,1.0f,0.0f)); });
}
