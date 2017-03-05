#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>
#include <functional>
#include <iostream>
#include "Util.hpp"
#include "Window_Inputs.hpp"
#include <deque>

// handles mouse buttons and keyboard
static std::deque<Key_Input> key_inputs;

// handles cursor pos movement
static std::deque<v2> cursor_inputs;

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

static void window_focus_callback(GLFWwindow* window, int focused) {
    if (focused) {
        window_gained_focus_ = true;
        window_lost_focus_ = false;
        // The window gained input focus
    }
    else {
        window_gained_focus_ = false;
        window_lost_focus_ = true;
        // The window lost input focus
    }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    key_inputs.emplace_back(key,action);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    key_inputs.push_back(Key_Input(button,action));
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    // The callback functions receives the cursor position, 
    // measured in screen coordinates but relative to the top-left corner of the window client area
    cursor_inputs.push_back(v2(xpos,ypos));
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

static void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

v2 Window_Inputs::windowSize() {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    return v2(width, height);
}

GLFWwindow* Window_Inputs::init_window(int x, int y) {
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        std::cerr << "glfw init failed" << "\n";
        // Initialization failed
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    window = glfwCreateWindow(x, y, "Sick window here", NULL, NULL);

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
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetWindowFocusCallback(window, window_focus_callback);

    glfwSetWindowPos(window, 500, 100);

    // Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
    glewExperimental = GL_TRUE;
    // Initialize GLEW to setup the OpenGL Function pointers
    glewInit();

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    cursor_pos.x = xpos;
    cursor_pos.y = ypos;

    disable_cursor();

    // turn off v-sync
    // do sleeping myself
    glfwSwapInterval(0);
    return window;
}

Window_Inputs::Window_Inputs() {
}

void Window_Inputs::disable_cursor() {
    cursor_disabled = true;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    cursorDelta(); // bin the movement from the switch, so don't get a lurch
}

void Window_Inputs::enable_cursor() {
    cursor_disabled = false;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Window_Inputs::toggle_cursor() {
    if (cursor_disabled) {
        enable_cursor();
    } else {
        disable_cursor();
    }
}

v2 Window_Inputs::cursorDelta() {
    // if queue empty, cursor hasn't moved
    if (cursor_inputs.empty() || !cursor_disabled) {
        return v2(zeroV);
    }

    const v2 latestPos = cursor_inputs.back(); 
    const v2 delta = cursor_pos - latestPos;
    cursor_pos = latestPos;
    cursor_inputs.clear();
    return delta;
}

GLFWwindow* Window_Inputs::getWindow() {
    return window;
}

void Window_Inputs::swapBuffers() {
    glfwSwapBuffers(window);
}

void Window_Inputs::processInput() {
    glfwPollEvents();
    
    // update all keys in keymap from inputs
    for (const auto& key_input: key_inputs) {
        keymap[key_input.key].setAction(key_input.action);
    }
    key_inputs.clear();
    
    for (auto& inp: keymap) {
        Input& in = inp.second;
        in.doAction();
    }
}

bool Window_Inputs::window_gained_focus() {
    bool stat = window_gained_focus_;
    window_gained_focus_ = false;
    return stat;
}

bool Window_Inputs::window_lost_focus() {
    bool stat = window_lost_focus_;
    window_lost_focus_ = false;
    return stat;
}

void Window_Inputs::close() {
    glfwDestroyWindow(window);
    glfwTerminate();
}
