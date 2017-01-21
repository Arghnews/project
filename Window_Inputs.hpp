#ifndef WINDOW_INPUTS_H
#define WINDOW_INPUTS_H
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>
#include <functional>

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
        GLFWwindow* window;
        std::map<int, Input> inputs;

    public:
        Window_Inputs();

        GLFWwindow* init_window();
        GLFWwindow* getWindow();

        void input(int key, const int action);
        void doAll();

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


        void close();
};

#endif
