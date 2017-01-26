#include "Camera.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // vulkan range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>

Camera::Camera() : Camera(0.0f,0.0f) {
		std::cout << "\n";
};
Camera::Camera(float x, float y)
	{
		std::cout << "Camera being made \n";
		cursorXCoord = x;
		cursorYCoord = y;
};

void Camera::setCursorXY(const float x, const float y) {
	cursorXCoord = x;
	cursorYCoord = y;
}
float Camera::cursorX() {
	return cursorXCoord;
}

float Camera::cursorY() {
	return cursorYCoord;
}

// deltaX/Z should be from -1 to 1, 1=90degrees
void Camera::turnObject(
		int WIDTH,
		int HEIGHT,
		double mouseX,
		double mouseY,
		glm::vec3& lookingAt) {

	int halfWidth = WIDTH/2;
	int halfHeight = HEIGHT/2;
	double xPos = mouseX/(double)halfWidth - 1.0;
	double yPos = 1.0 - mouseY/(double)halfHeight;
	float oldCursorX = cursorX();
	float oldCursorY = cursorY();
	double deltaX = xPos-oldCursorX;
	double deltaZ = yPos-oldCursorY;
	//cam->turnObject(deltaX,deltaZ);
	static const float PI_BY_EIGHTEEN = M_PI/18.0f; 
	static const float PI_BY_EIGHTEEN_TIMES_SEVENTEEN = 17.0f * M_PI/18.0f; 
	glm::vec3 lookingAtCopy = lookingAt;

	// don't want to move too far, ie. from 169 degrees up round to bottom, if such a move, won't do it
	float newAngle = std::acos(glm::dot(lookingAtCopy,up))-deltaZ;
	if (newAngle > PI_BY_EIGHTEEN && newAngle < PI_BY_EIGHTEEN_TIMES_SEVENTEEN) {
		lookingAtCopy = glm::normalize(glm::rotate(lookingAtCopy, (float)deltaZ, glm::cross(lookingAtCopy,up))); // z
	}
	lookingAtCopy = glm::normalize(glm::rotate(lookingAtCopy, -(float)(deltaX), up)); // x
	lookingAt = lookingAtCopy;

	cursorXCoord += deltaX;
	cursorYCoord += deltaZ;
}

