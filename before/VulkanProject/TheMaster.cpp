#include "TheMaster.hpp"
#include "MyVulkan.hpp"
#include <iostream>
#include "Camera.hpp"
#include "World.hpp"
#include "Physics.hpp"
#include <thread>

int main() {
	TheMaster app;

	try {
		app.run();
		std::cout << "Hi there \n";	
	} catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

TheMaster::TheMaster() : 
	world(new World),
	MAIN_CAMERA("Camera")
	{
	window = initWindow();
	vulkan.init(window,WIDTH,HEIGHT);
	input.setWorld(world);
	input.setWindowPtr(window);
	vulkan.initVulkan();

	double oldCursorX, oldCursorY;
	glfwGetCursorPos(window, &oldCursorX, &oldCursorY);
	// normalize values 
	oldCursorX = oldCursorX/(float)(WIDTH/2.0f) - 1.0f;
	oldCursorY = 1.0f - oldCursorY/(float)(HEIGHT/2.0f);

	Thing* thing = new Thing(MAIN_CAMERA);
	Camera* cam = new Camera(oldCursorX,oldCursorY);
	thing->camera = cam;
	world->addObject(thing);
}

void TheMaster::run() {
    double simsPerSec = 60.0;
    double fps = 60.0;
    double t = 0.0;
    double dt = 1000.0/simsPerSec;
    const double startTime = time_now();
    double currentTime = time_now();
    double accumulator = 0.0;
    int i = 0;
    int r = 0;
    //State previous;
    //Thing current;
	while (!glfwWindowShouldClose(window)) {
        double newTime = time_now();
        
        // ms
        double frameTime = (newTime - currentTime)/1000000.0f;
        // won't work below 4fps really
        currentTime = newTime;

        // add frameTime into acc
        accumulator += frameTime;
        // 
        while ( accumulator >= dt )
        {
			glfwPollEvents(); // input processed here
			input.getFuckedWithYourPatternsIlettIAmMakingAMonolith_ProcessKeyInput();
			glm::vec3 booleanAccel = input.normalForceMoveVectorFromKeys();

			moveThingRelativeToSelf(MAIN_CAMERA,booleanAccel);

			input.turnObjectFromMouse(WIDTH,HEIGHT,MAIN_CAMERA);
            //previousState = currentState;
            world->physics.integrate( MAIN_CAMERA, t, dt );
            //std::this_thread::sleep_for(std::chrono::milliseconds(4));
            t += dt;
            accumulator -= dt;
        }

        //std::this_thread::sleep_for(std::chrono::milliseconds(2));
        //const double alpha = accumulator / dt;
        //State state = currentState * alpha + previousState * ( 1.0 - alpha );
        //render( state );
		const Thing* camera = world->object(MAIN_CAMERA);
		vulkan.updateUniformBuffer(
				camera->position,
				camera->lookingAt,
				world->up
				);
		vulkan.drawFrame();
        
        // my code to limit this to 60fps
        const double loopMaxTime = 1000000000.0/fps;
        double nsThisLoop = time_now() - newTime;

        long nanoSleep = static_cast<long>(loopMaxTime - nsThisLoop);
        nanoSleep = std::max(0l,nanoSleep);

		//std::cout << "Sleeping for " << nanoSleep/1e6f << "ms \n";
        std::this_thread::sleep_for(std::chrono::nanoseconds(nanoSleep));
        //std::cout << "\n";
	}

	vulkan.cleanup();
}

// essentially, if y=1 then will move forward at object's acceleration attrib pace
// ie. booleanAccel = where x=(-1,0 or 1) (x,x,x) to move forward at normal pace
void TheMaster::moveThingRelativeToSelf(std::string objectName, const glm::vec3& booleanAccel) {
	Thing* thing = world->object(objectName);
	const glm::vec3 forwardDir = glm::normalize(thing->lookingAt);
	const glm::vec3 forward = forwardDir * thing->acceleration.y * booleanAccel.y;

	const glm::vec3 sidewaysDir = glm::normalize(glm::cross(forwardDir,world->up));
	const glm::vec3 sideways = sidewaysDir * thing->acceleration.x * booleanAccel.x;

	const glm::vec3 relativeUpDir = glm::cross(sidewaysDir,forwardDir);
	const glm::vec3 relativeUp = relativeUpDir * thing->acceleration.z * booleanAccel.z;

	thing->add_force(forward);
	thing->add_force(sideways);
	thing->add_force(relativeUp);
}

static void windowCloseCallback(GLFWwindow* window) {
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void onWindowResized(GLFWwindow* window, int width, int height) {
	if (width == 0 || height == 0) return;

	TheMaster* app = reinterpret_cast<TheMaster*>(glfwGetWindowUserPointer(window));
	app->masterfulResize(width,height);
}

void TheMaster::masterfulResize(int width, int height) {
	vulkan.recreateSwapchain();
	vulkan.setWindowSize(width,height);
}

GLFWwindow* TheMaster::initWindow() {

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	// locks cursor to window
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetWindowUserPointer(window, this);

	glfwSetWindowSizeCallback(window, onWindowResized);
	glfwSetWindowCloseCallback(window, windowCloseCallback);

	input.init(window);

	return window;
}

TheMaster::~TheMaster() {
	delete world;
}
