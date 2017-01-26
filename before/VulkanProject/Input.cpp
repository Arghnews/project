#include "Input.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <unordered_map>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // vulkan range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>
#include <string>

Input::Input() : window(window) {
	for (int i=0; i<350; i++) {
		keyboard[i] = false;
	}
}

void Input::setWorld(World* SOMETHINGELSE) {
	world = SOMETHINGELSE;
}

// just pushes the key event onto the queue
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	KeyPress event = KeyPress{};
	event.key = key;
	event.action = action;
	keyPresses.push(event);
}

// just pushes the cursor movement onto queue
static void cursorPosCallback(GLFWwindow* window, double xPos, double yPos) {
	CursorPos pos = CursorPos{};
	pos.x = xPos;
	pos.y = yPos;
	mouseMovements.push(pos);
}

void Input::init(GLFWwindow* window) {
	glfwSetKeyCallback(window, keyCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
}

// takes mouse/keyboard events from queue and does shit with them
void Input::getFuckedWithYourPatternsIlettIAmMakingAMonolith_ProcessKeyInput()
{
	
	while (!keyPresses.empty()) {
		KeyPress keyP = keyPresses.front();
		int key = keyP.key;
		int action = keyP.action;
		keyPresses.pop();
		auto keyUpdate = [&] (const int& action, const int& key) -> void {
			if (action == GLFW_PRESS) {
				keyboard[key]=true;
			} else if (action == GLFW_RELEASE) {
				keyboard[key]=false;
			}
		};
		switch (key) {
			case GLFW_KEY_W:
				keyUpdate(action,GLFW_KEY_W); break;

			case GLFW_KEY_S:
				keyUpdate(action,GLFW_KEY_S); break;

			case GLFW_KEY_A:
				keyUpdate(action,GLFW_KEY_A); break;

			case GLFW_KEY_D:
				keyUpdate(action,GLFW_KEY_D); break;

			case GLFW_KEY_UP:
				keyUpdate(action,GLFW_KEY_UP); break;

			case GLFW_KEY_DOWN:
				keyUpdate(action,GLFW_KEY_DOWN); break;

			case GLFW_KEY_LEFT_SHIFT:
				keyUpdate(action,GLFW_KEY_LEFT_SHIFT); break;

			case GLFW_KEY_ESCAPE:
				keyUpdate(action,GLFW_KEY_ESCAPE); break;

			case GLFW_KEY_RIGHT_SHIFT:
				keyUpdate(action,GLFW_KEY_RIGHT_SHIFT);
				if (action == GLFW_PRESS) {
				}
				break;
		}
	}

	if (keyboard[GLFW_KEY_LEFT_SHIFT]) {
		//accel = &fastAccel;
	} else {
		//accel = &normalAccel;
	}

	if (keyboard[GLFW_KEY_ESCAPE]) {
		quit();
	}

}

glm::vec3 Input::normalForceMoveVectorFromKeys() {
	// first resolve accel vectors
	// then add that to velocity
	// then add that to displacement

	glm::vec3 movementAccel = glm::vec3(0.0f,0.0f,0.0f);

	// ensure direction vector is normalized

	// velocity of me is x,y,z components corresponding to axis
	// velo = rates.delta()*accel mag * direction basically
	if (keyboard[GLFW_KEY_W]) {
		movementAccel.y += 1.0f;
	}
	if (keyboard[GLFW_KEY_S]) {
		movementAccel.y -= 1.0f;
	}
	if (keyboard[GLFW_KEY_A]) {
		movementAccel.x -= 1.0f;
	}
	if (keyboard[GLFW_KEY_D]) {
		movementAccel.x += 1.0f;
	}
	if (keyboard[GLFW_KEY_UP]) {
		movementAccel.z += 1.0f;
	}
	if (keyboard[GLFW_KEY_DOWN]) {
		movementAccel.z -= 1.0f;
	}
	return movementAccel;
}

void Input::turnObjectFromMouse(int WIDTH, int HEIGHT, std::string cameraName) {
	// moves a thing's camera, also updates it's looking at vector
	Thing* thing = world->object(cameraName);
	Camera* cam = thing->camera;
	glm::vec3& lookingAt = thing->lookingAt;
	
	while (!mouseMovements.empty()) {
		const CursorPos& pos = mouseMovements.front();
		cam->turnObject(
				WIDTH,
				HEIGHT,
				pos.x,
				pos.y,
				lookingAt);
		mouseMovements.pop();
	}
}

void Input::setWindowPtr(GLFWwindow* daWindow) {
	window = daWindow;
}

void Input::quit() {
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}

