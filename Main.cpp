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

#include "Util.hpp"
#include "Shader.hpp"
#include "Window_Inputs.hpp"
#include "Camera.hpp"
#include "data.hpp"

// extern in Window_Inputs cpp
Window_Inputs inputs;

void set_keyboard(Window_Inputs& inputs, GLFWwindow* window, Camera& camera);

int main() {

    GLFWwindow* window = inputs.init_window(1440, 1440);

    Camera camera;

    set_keyboard(inputs,window,camera);
    
    Shader normalShader("vertex.shader", "fragment.shader");
    Shader lightShader("light.vertex.shader", "light.fragment.shader");

    // cube --
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // Bind the Vertex Array Object first, then bind and set vertex bufer(s) and attribute pointer(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // Normals attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);


    glBindVertexArray(0); // Unbind VAO

    // light --
    GLuint lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    // We only need to bind to the VBO, the container's VBO's data already contains the correct data.
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Set the vertex attributes (only position data for our lamp)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0); 


    while (!glfwWindowShouldClose(window)) {

        inputs.processInput(); // polls input and executes action based on that

        const v2 mouseDelta = inputs.cursorDelta();
        camera.rotate(mouseDelta);

        // Render
        // Clear the colorbufer
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // local space -> world space -> view space -> clip space -> screen space
        //          model matrix   view matrix  projection matrix   viewport transform
        // Vclip = Mprojection * Mview * Mmodel * Vlocal

        v3 lightPos(2.0f,0.0f,0.0f);
        m4 model;

        // normal cube
        // --
        normalShader.use();
        GLint matAmbientLoc  = glGetUniformLocation(normalShader.Program, "material.ambient");
        GLint matDiffuseLoc  = glGetUniformLocation(normalShader.Program, "material.diffuse");
        GLint matSpecularLoc = glGetUniformLocation(normalShader.Program, "material.specular");
        GLint matShineLoc    = glGetUniformLocation(normalShader.Program, "material.shininess"); 

        GLint lightAmbientLoc  = glGetUniformLocation(normalShader.Program, "light.ambient");
        GLint lightDiffuseLoc  = glGetUniformLocation(normalShader.Program, "light.diffuse");
        GLint lightSpecularLoc = glGetUniformLocation(normalShader.Program, "light.specular");
          
        glUniform3f(lightAmbientLoc,  0.2f, 0.2f, 0.2f);
        glUniform3f(lightDiffuseLoc,  0.5f, 0.5f, 0.5f); // Let's darken the light a bit to fit the scene
        glUniform3f(lightSpecularLoc, 1.0f, 1.0f, 1.0f);
          
        glUniform3f(matAmbientLoc,  0.05375f,   0.05f, 0.06625f);
        glUniform3f(matDiffuseLoc,  0.18275f, 0.17f, 0.22525f);
        glUniform3f(matSpecularLoc, 0.332741f, 0.328634f, 0.346435f);
        glUniform1f(matShineLoc,    0.3f * 128.0f);

        GLint lightPosLoc = glGetUniformLocation(normalShader.Program, "light.position");
        GLint viewPosLoc = glGetUniformLocation(normalShader.Program, "viewPos");
        // Don't forget to 'use' the corresponding shader program first (to set the uniform)
        glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z); 
        glUniform3f(viewPosLoc, camera.pos().x, camera.pos().y, camera.pos().z);

        m4 view = camera.update();
        float aspectRatio = inputs.windowSize().x / inputs.windowSize().y;
        m4 projection = glm::perspective(glm::radians(90.0f), aspectRatio, 0.1f, 200.0f);
        GLuint modelLoc = glGetUniformLocation(normalShader.Program, "model");
        GLuint viewLoc = glGetUniformLocation(normalShader.Program, "view");
        GLuint projectionLoc = glGetUniformLocation(normalShader.Program, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);
        model = m4();
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        glBindVertexArray(0);
        // --

        // light source
        // --
        lightShader.use();
        modelLoc = glGetUniformLocation(lightShader.Program, "model");
        viewLoc = glGetUniformLocation(lightShader.Program, "view");
        projectionLoc = glGetUniformLocation(lightShader.Program, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(lightVAO);
        model = m4();
        model = glm::translate(model, lightPos);
        model = glm::scale(model, v3(0.2f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        glBindVertexArray(0);
        // --

        inputs.swapBuffers(); // swaps buffers
    }


    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    inputs.close();
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
