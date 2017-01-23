#ifndef WINDOW_INPUTS_H
#define WINDOW_INPUTS_H
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>
#include <functional>
#include "Util.hpp"

typedef std::function<void()> void_func;
static const void_func NOPfunc = ([](){});

class Input {
    private:
        static const int NOP = -1;
        int action;

    public:

        void_func pressed_func;
        void_func released_func;
        void_func repeated_func;

        Input();
        void setAction(const int action_in);
        void doAction();

};

class Window_Inputs {
    private:
        v2 cursor_pos;
        v2 last_cursor_pos;
        GLFWwindow* window;
        std::map<int, Input> inputs;

    public:
        Window_Inputs();

        GLFWwindow* init_window(int x=1024, int y=768);
        GLFWwindow* getWindow();
        v2 windowSize();

        void input(int key, const int action);
        void swapBuffers();
        void processInput();

        void cursor(const double& xPos, const double& yPos);
        v2 cursorDelta();

        template <typename F>
        void setFunc(int key, int action, F f) {
            switch (action) {
                case GLFW_PRESS:
                    inputs[key].pressed_func = f;
                    break;
                case GLFW_REPEAT:
                    inputs[key].repeated_func = f;
                    break;
                case GLFW_RELEASE:
                    inputs[key].released_func = f;
                    break;
            }
        }

        template <typename F>
        void setFunc2(int key, F f) {
            setFunc(key,GLFW_PRESS,f);
            setFunc(key,GLFW_REPEAT,f);
        }


        void close();
};

#endif
