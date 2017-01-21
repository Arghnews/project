#include <GL/glew.h>
#include <GLFW/glfw3.h>

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

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        std::cout << "Pressed right mouse button" << "\n";
    }
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

static void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

static GLFWwindow* init_window() {
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    return window;
}

static void close_window(GLFWwindow* window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

/* int main() {
    GLFWwindow* window = init_window();
    while (!glfwWindowShouldClose(window)) {
        // Keep running
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    close_window(window);
} */
