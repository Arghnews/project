#ifndef THE_MASTER
#define THE_MASTER

#include "MyVulkan.hpp"
#include "Input.hpp"
#include "Camera.hpp"
#include "World.hpp"

class TheMaster {
	public:
		TheMaster();
		~TheMaster();
		void run();
		void masterfulResize(int width, int height);

	private:
		void moveThingRelativeToSelf(std::string objectName, const glm::vec3& booleanAccel);
		GLFWwindow* initWindow();
		World* world;
		const std::string MAIN_CAMERA;
		Input input;
		HelloTriangleApplication vulkan;
		GLFWwindow* window;
		int WIDTH = 1280;
		int HEIGHT = 1024;
};


#endif
