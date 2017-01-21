#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Window_Inputs.hpp"

Window_Inputs inputs;

void set_keyboard(Window_Inputs& w_in, GLFWwindow* window);

int main() {
    GLFWwindow* window = inputs.init_window();
    set_keyboard(inputs, window);
    while (!glfwWindowShouldClose(window)) {
        // Keep running
        inputs.doAll();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    inputs.close();
}

void set_keyboard(Window_Inputs& w_in, GLFWwindow* window) {
    w_in.setFunc(GLFW_KEY_ESCAPE,GLFW_PRESS,[&] () {std::cout << "You pressed escape\n"; });
    w_in.setFunc(GLFW_KEY_ESCAPE,GLFW_REPEAT,[&] () {std::cout << "You held escape\n"; });
    w_in.setFunc(GLFW_KEY_ESCAPE,GLFW_RELEASE,[=] () {glfwSetWindowShouldClose(window, GLFW_TRUE); });
    w_in.setFunc(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, [&] () {std::cout << "Left mouse\n"; });
}
