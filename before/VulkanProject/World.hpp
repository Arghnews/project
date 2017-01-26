#ifndef MY_WORLD
#define MY_WORLD
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // vulkan range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <map>
#include <string>
#include <memory>

#include "Helper.hpp"
#include "Thing.hpp"
#include "Physics.hpp"

class World {
	public:
		World();
		Physics physics;
		void addObject(Thing* thingPtr);
		void turnObject(std::string name, const float& deltaX, const float& deltaZ);
		Thing* object(const std::string name);
		const glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
		~World();

	private:
		std::map<std::string,Thing*> things;
		static constexpr float decelConst = 0.0f;
		static constexpr float decel = 0.05f;
		static constexpr glm::vec3 airRes = glm::vec3(decel,decel,decel); // deceleration constant
		static constexpr float mu = 0.25f;
		static constexpr glm::vec3 friction = glm::vec3(mu,mu,mu);
		glm::vec3 worldDecel(const glm::vec3& velo);
};

#endif
