#ifndef MY_INPUT
#define MY_INPUT
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <queue>
#include <unordered_map>
#include "World.hpp"
#include <string>

struct KeyPress {
	int key;
	int action;
};
struct CursorPos {
	double x;
	double y;
};

// these must be effectively static and not bound to class as they are accessed from static
// function as glfw insists on static functions...
static std::queue<KeyPress> keyPresses;
static std::queue<CursorPos> mouseMovements;

class Input {
	public:
		Input();
		void setWindowPtr(GLFWwindow* window);
		void setWorld(World* world);
		void getFuckedWithYourPatternsIlettIAmMakingAMonolith_ProcessKeyInput();
		glm::vec3 normalForceMoveVectorFromKeys();
		void turnObjectFromMouse(int WIDTH, int HEIGHT, std::string cameraName);
		void quit();
		void init(GLFWwindow* window);

	private:
		World* world;
		GLFWwindow* window;
		std::unordered_map<int,bool> keyboard;

};

#endif
