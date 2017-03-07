#ifndef WINDOW_INPUTS_H
#define WINDOW_INPUTS_H
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>
#include <functional>
#include <string>
#include "Util.hpp"

typedef std::function<void()> void_func;
static const void_func NOPfunc = ([](){});

static bool window_gained_focus_;
static bool window_lost_focus_;

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

        GLFWwindow* init_window(std::string window_name="Sick window name here", int x=800, int y=600);
        GLFWwindow* getWindow();
        v2 windowSize();
        
        // reading from this resets the static bool
        bool window_gained_focus();
        bool window_lost_focus();

        void toggle_cursor();
        void disable_cursor();
        void enable_cursor();
        bool cursor_disabled;

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

        // sets to REPEAT for as long as you hold key
        template <typename F>
        void setFunc2(int key, F f) {
            setFunc(key,GLFW_PRESS,f);
            setFunc(key,GLFW_REPEAT,f);
        }

        void close();
};

#endif
