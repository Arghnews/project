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

Window_Inputs inputs;

void set_keyboard(Window_Inputs& w_in, GLFWwindow* window, Camera& c);

int main() {

    GLFWwindow* window = inputs.init_window(1440, 1440);

    Camera camera;
    
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

    Shader shaderProgram("vertex.shader", "fragment.shader");

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // Bind the Vertex Array Object first, then bind and set vertex bufer(s) and attribute pointer(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)(0 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); // Unbind VAO

    while (!glfwWindowShouldClose(window)) {

        const v2 mouseDelta = inputs.cursorDelta();
        camera.rotate(mouseDelta);

        // Render
        // Clear the colorbufer
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);
        shaderProgram.use();

        //
        // local space -> world space -> view space -> clip space -> screen space
        //          model matrix   view matrix  projection matrix   viewport transform
        // Vclip = Mprojection * Mview * Mmodel * Vlocal

        float aspectRatio = inputs.windowSize().x / inputs.windowSize().y;

        m4 model;
        m4 trans;
        m4 rotateM; //= glm::mat4_cast(qua);
        m4 scale;

        //trans = glm::translate(trans, pos);
        //scale = glm::scale(scale, sca);
        //model = trans * rotateM * scale;

        m4 view = camera.update();

        m4 projection;
        projection = glm::perspective(glm::radians(90.0f), aspectRatio, 0.1f, 200.0f);
        //projection = glm::ortho(-3.0f,3.0f,-3.0f,3.0f,0.1f, 100.0f);

        GLint modelLoc = glGetUniformLocation(shaderProgram.Program, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        GLint viewLoc = glGetUniformLocation(shaderProgram.Program, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        GLint projectionLoc = glGetUniformLocation(shaderProgram.Program, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glDrawArrays(GL_TRIANGLES, 0, vertices.size());

        inputs.loop(); // polls loop and executes functions
    }


    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    inputs.close();
}

