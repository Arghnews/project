#include "World.hpp"
#include "Helper.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // vulkan range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>
#include <functional>
#include <memory>
#include "Thing.hpp"
#include "Camera.hpp"
#include "Physics.hpp"

World::World() : physics(this) {
}

void World::addObject(Thing* thingPtr) {
	if (things.count(thingPtr->name) != 0) {
		throw std::runtime_error("Attempted to create two objects with same name");
	}
	//things[name] = std::unique_ptr<Thing>(new Thing(lookingAt,position,velocity,accel,name));
	things[thingPtr->name] = thingPtr;
}

Thing* World::object(const std::string name) {
	//std::cout << &things << "\n";
	if (things.count(name) == 0) {
		throw std::runtime_error("Attempted to retrieve object that doesn't exist");
	}
	return things[name];
}

// takes mouse/keyboard events from queue and does shit with them
/*
void World::moveObject(
		Thing* thing,
		glm::vec3 accel
		) {

	// ensure direction vector is normalized
	thing->lookingAt = glm::normalize(thing->lookingAt);
	glm::vec3 sideways = glm::cross(thing->lookingAt,up); // mag's of both should already be normalized
	glm::vec3 relativeUp = glm::cross(sideways,thing->lookingAt);

	// first resolve accel vectors
	// then add that to thing->velocity
	// then add that to displacement

	glm::vec3 movementAccel = glm::vec3(0.0f,0.0f,0.0f);

	// thing->velocity of me is x,y,z components corresponding to axis
	// velo = rates.delta()*accel mag * direction basically
	movementAccel += accel.y*thing->lookingAt; // forward/back
	//movementAccel -= accel.y*thing->lookingAt;
	movementAccel += accel.x*sideways; // left/right
	//movementAccel += accel.x*sideways;
	movementAccel += accel.z*relativeUp; // up/down
	//movementAccel -= accel.z*relativeUp;

	// currently does not take any mass into account
	//worldDecel(thing->velocity);
	// xd,yd,zd are amounts (+ve) to decrease the thing->velocity by
	// everything needs to be multiplied by time see equations
	// positive decel value of accel, proportional to square of speed
	glm::vec3 movementDecel = worldDecel(thing->velocity);
	//velo.x = std::min(velo.x+xd,std::max(0.0f,velo.x-xd));
	//velo.y = std::min(velo.y+yd,std::max(0.0f,velo.y-yd));
	//velo.z = std::min(velo.z+zd,std::max(0.0f,velo.z-zd));
	thing->velocity += (movementAccel - movementDecel);

	thing->position += thing->velocity;

	bool p = false;
	if (p) {
		std::cout << "Accel " << printVec(movementAccel) << "\n";
		std::cout << "Decel " << printVec(movementDecel) << "\n";
		std::cout << "My speed " << sqrt(thing->velocity.x*thing->velocity.x+thing->velocity.y*thing->velocity.y+thing->velocity.z*thing->velocity.z) << "\n";
		std::cout << "Velocity " << printVec(thing->velocity);
		std::cout << "\n";
	}
	// world decel force should be applied here
	// this is where movement actually happens

}
*/

World::~World() {
	std::cout << "Map being deleted!" << "\n";
	for(std::map<std::string, Thing*>::iterator itr = things.begin(); itr != things.end(); itr++) {
		delete itr->second;
	}
}
