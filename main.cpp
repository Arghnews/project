#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

static void error_callback(int error, const char* description) {
        fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        std::cout << "You pressed E\n";
    } else if (key == GLFW_KEY_E && action == GLFW_REPEAT) {
        std::cout << "Repeated E\n";
    } else if (key == GLFW_KEY_E && action == GLFW_RELEASE) {
        std::cout << "Released E\n";
    }
}

static void window_close_callback(GLFWwindow* window) {
    glfwSetWindowShouldClose(window, GLFW_FALSE);
}

static void window_size_callback(GLFWwindow* window, int width, int height) {
    // window resized
    //std::cout << "Window size now " << width << " by " << height << "\n";
}

static void window_focus_callback(GLFWwindow* window, int focused)
{
    if (focused) {
        // The window gained input focus
    }
    else {
        // The window lost input focus
    }
}

int main() {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        std::cerr << "glfw init failed" << "\n";
        // Initialization failed
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(1024, 768, "Sick window here", NULL, NULL);
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

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    while (!glfwWindowShouldClose(window)) {
        // Keep running
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}

