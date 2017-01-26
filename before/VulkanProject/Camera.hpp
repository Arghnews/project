#ifndef MY_CAMERA
#define MY_CAMERA
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // vulkan range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <string>
#include "Helper.hpp"

class Camera {
	public:
		float cursorX();
		float cursorY();
		Camera();
		Camera(float cursorX, float cursorY);
		void setCursorXY(const float x, const float y);
		void turnObject(
				int WIDTH,
				int HEIGHT,
				double mouseX,
				double mouseY,
				glm::vec3& lookingAt);

		static constexpr glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
	private:
		float cursorXCoord;
		float cursorYCoord;

};

#endif
