#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>
#include <functional>
#include <iostream>
#include "Window_Inputs.hpp"

extern Window_Inputs inputs; // should be defined in main

Input::Input() : 
    action(NOP),
    pressed_func(NOPfunc),
    released_func(NOPfunc),
    repeated_func(NOPfunc) {
    }

void Input::setAction(const int action_in) {
    action = action_in;
}

void Input::doAction() {
    switch (action) {
        case GLFW_PRESS:
            pressed_func();
            action = GLFW_REPEAT;
            break;
        case GLFW_RELEASE:
            released_func();
            action = NOP;
            break;
        case GLFW_REPEAT:
            repeated_func();
            break;
        default:
            break;
    }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    inputs.input(key, action);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    inputs.input(button, action);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    // The callback functions receives the cursor position, 
    // measured in screen coordinates but relative to the top-left corner of the window client area
    std::cout << "Cursor pos: (" << xpos << "," << ypos << ")" << "\n";
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    //std::cout << "Scrolling by " << yoffset << "\n";
}

static void window_close_callback(GLFWwindow* window) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void window_size_callback(GLFWwindow* window, int width, int height) {
    // window resized
    //std::cout << "Window size now " << width << " by " << height << "\n";
}

static void window_focus_callback(GLFWwindow* window, int focused) {
    if (focused) {
        // The window gained input focus
    }
    else {
        // The window lost input focus
    }
}

static void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

GLFWwindow* Window_Inputs::init_window() {
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        std::cerr << "glfw init failed" << "\n";
        // Initialization failed
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    window = glfwCreateWindow(1024, 768, "Sick window here", NULL, NULL);

    if (!window) {
        // Window or OpenGL context creation failed
        glfwTerminate();
        std::cerr << "Window creation failed" << "\n";
        exit(1);
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowCloseCallback(window, window_close_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetWindowFocusCallback(window, window_focus_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSwapInterval(1);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    return window;
}

Window_Inputs::Window_Inputs() {
}

GLFWwindow* Window_Inputs::getWindow() {
    return window;
}

void Window_Inputs::input(int key, const int action) {
    inputs[key].setAction(action);
}

void Window_Inputs::doAll() {
    for (auto& inp: inputs) {
        Input& in = inp.second;
        in.doAction();
    }
}

void Window_Inputs::close() {
    glfwDestroyWindow(window);
    glfwTerminate();
}
