#ifndef WINDOW_INPUTS_H
#define WINDOW_INPUTS_H
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>
#include <functional>
#include "Util.hpp"

typedef std::function<void()> void_func;
static const void_func NOPfunc = ([](){});

struct Key_Input {
    const int key;
    const int action;
    Key_Input(const int& key, const int& action) :
        key(key), action(action) {
    }
};

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
        GLFWwindow* window;
        std::map<int, Input> keymap;

    public:
        Window_Inputs();

        GLFWwindow* init_window(int x=800, int y=600);
        GLFWwindow* getWindow();
        v2 windowSize();

        void swapBuffers();
        void processInput();

        v2 cursorDelta();

        template <typename F>
        void setFunc(int key, int action, F f) {
            switch (action) {
                case GLFW_PRESS:
                    keymap[key].pressed_func = f;
                    break;
                case GLFW_REPEAT:
                    keymap[key].repeated_func = f;
                    break;
                case GLFW_RELEASE:
                    keymap[key].released_func = f;
                    break;
            }
        }

        template <typename F>
        void setFunc1(int key, F f) {
            setFunc(key,GLFW_PRESS,f);
        }

        template <typename F>
        void setFunc2(int key, F f) {
            setFunc(key,GLFW_PRESS,f);
            setFunc(key,GLFW_REPEAT,f);
        }

        void close();
};

#endif
