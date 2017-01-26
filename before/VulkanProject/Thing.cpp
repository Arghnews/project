#include "Thing.hpp"

#include <vector>
#include <string>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // vulkan range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "Helper.hpp"

// for now won't copy the camera, don't need to anyway
Thing::Thing(const Thing& rhs) : Thing (
		rhs.position,
		rhs.lookingAt,
		rhs.momentum,
		rhs.acceleration,
		rhs.name,
		rhs.mass
		){
	forces = rhs.forces;
}

Thing::Thing(std::string name) : Thing (
		glm::vec3(2.0f,2.0f,2.0f),
		glm::vec3(-1.0f,-1.0f,-1.0f),
		glm::vec3(0.0f,0.0f,0.0f),
		glm::vec3(0.02f,0.02f,0.02f),
		name,
		1.0f
		) {
}

Thing::Thing (
		glm::vec3 position,
		glm::vec3 lookingAt,
		glm::vec3 momentum,
		glm::vec3 acceleration,
		const std::string name,
		float m
		) :
		position(position),
		lookingAt(lookingAt),
		momentum(momentum),
		acceleration(acceleration),
		name(name),
		mass(m),
		inverseM(1.0f/m),
		camera(nullptr)
	{
}

float Thing::speed() const {
	return glm::length(velocity());
}

glm::vec3 Thing::net_force() {
	// clears forces vector
	glm::vec3 netForce = glm::vec3(0.0f,0.0f,0.0f);
	for (const auto& force : forces) {
		netForce += force;
	}
	forces.clear();
	return netForce;
}

void Thing::add_force(const glm::vec3 force) {
	forces.push_back(force);
}

glm::vec3 Thing::velocity() const {
	return momentum * inverse_mass();
}
float Thing::inverse_mass() const {
	return inverseM;
}

Thing::~Thing() {
	if (camera != nullptr) {
		delete camera;
	}
}
