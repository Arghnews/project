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
void set_keyboard(Window_Inputs& inputs, GLFWwindow* window, Camera& camera);

int main() {

    Window_Inputs inputs;

    GLFWwindow* window = inputs.init_window(1440, 1024);

    Camera camera;

    set_keyboard(inputs,window,camera);

    G_Cuboid cube(&vertices,"shaders/vertex.shader","shaders/fragment.shader");

    const long fps_max = 60l;
    
    const long tickrate = 100l;
    const long dt = (1e6l)/tickrate; // run at tickrate 100

    long t = 0l;
    long currentTime = timeNowMicros();
    long acc = 0l;

    /*
    Actor me(&vertices, "shaders/vertex.shader",
            "shaders/fragment.shader", v3(0.0f,0.5f,0.0f),
            10.0f);
    P_State& my_phys = me.state_to_change();
    Physics phys;
    phys.integrate(my_phys, t, dt);
    const L_Cuboid& my_cub = me.logical_cuboid();
    m4 view = me.get_camera().viewMatrix(me.get_state().position);

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

        long newTime = timeNowMicros();
        long frameTime = newTime - currentTime;
        currentTime = newTime;
        acc += frameTime;

        // process inputs, change world
        inputs.processInput(); // polls input and executes action based on that

        const v2 mouseDelta = inputs.cursorDelta();
        camera.rotate(mouseDelta);

        // simulate world
        while (acc >= dt) {
            //integrate(state, t, dt);
            acc -= dt;
            t += dt;
        }

        // Render -- -- --
        // local space -> world space -> view space -> clip space -> screen space
        //          model matrix   view matrix  projection matrix   viewport transform
        // Vclip = Mprojection * Mview * Mmodel * Vlocal
        gl_loop_start();

        cube.bindBuffers();

        cube.useShader();

        m4 model;
        m4 view = camera.viewMatrix();
        float aspectRatio = inputs.windowSize().x / inputs.windowSize().y;
        m4 projection = glm::perspective(glm::radians(90.0f), aspectRatio, 0.1f, 200.0f);
        GLuint viewLoc = glGetUniformLocation(cube.shaderProgram(), "view");
        GLuint projectionLoc = glGetUniformLocation(cube.shaderProgram(), "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(cube.VAO);

        GLuint modelLoc = glGetUniformLocation(cube.shaderProgram(), "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, cube.drawSize());

        glBindVertexArray(0);
        glUseProgram(0);
        // Render end -- -- --
        
        // sleep if fps would be > fps_max
        long spareFrameTime = 1e6l / fps_max - (timeNowMicros() - newTime);
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

void set_keyboard(Window_Inputs& inputs, GLFWwindow* window, Camera& camera) {
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_PRESS,[&] () {std::cout << "You pressed escape\n"; });
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_REPEAT,[&] () {std::cout << "You held escape\n"; });

    inputs.setFunc(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, [&] () {std::cout << "Left mouse\n"; });

    // must be capture by value here
    inputs.setFunc(GLFW_KEY_ESCAPE,GLFW_RELEASE,[=] () {glfwSetWindowShouldClose(window, GLFW_TRUE); });

    // camera
    inputs.setFunc2(GLFW_KEY_W,[&] () {camera.move(FORWARD); });
    inputs.setFunc2(GLFW_KEY_S,[&] () {camera.move(BACKWARD); });
    inputs.setFunc2(GLFW_KEY_A,[&] () {camera.move(LEFT); });
    inputs.setFunc2(GLFW_KEY_D,[&] () {camera.move(RIGHT); });
    inputs.setFunc2(GLFW_KEY_UP,[&] () {camera.move(UP); });
    inputs.setFunc2(GLFW_KEY_DOWN,[&] () {camera.move(DOWN); });

    inputs.setFunc(GLFW_KEY_LEFT_SHIFT,GLFW_PRESS,[&] () {camera.toggleSpeed(); });
}
