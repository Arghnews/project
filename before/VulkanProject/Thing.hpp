#ifndef MY_THING
#define MY_THING
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // vulkan range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <string>
#include <vector>
#include "Helper.hpp"
#include "Camera.hpp"

class Thing {
	public:
		Thing(const Thing& rhs);
		Thing(const std::string name);
		Thing(
				glm::vec3 position,
				glm::vec3 lookingAt,
				glm::vec3 momentum,
				glm::vec3 acceleration,
				const std::string name,
				float mass
		);
		glm::vec3 velocity() const;
		float inverse_mass() const;
		void add_force(const glm::vec3 force);
		float speed() const;
		glm::vec3 net_force();
		glm::vec3 position;
		glm::vec3 lookingAt; // normalized, relative
		glm::vec3 momentum;
		glm::vec3 acceleration;
		const std::string name;
        float mass;      // mass
        float inverseM;  // 1/m
		virtual ~Thing();
		
		std::vector<glm::vec3>forces;
		Camera* camera;

		static constexpr glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
	protected:
};

#endif
