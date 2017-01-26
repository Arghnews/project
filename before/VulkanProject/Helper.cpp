#include "Helper.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // vulkan range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

std::string printVec(glm::vec3 v) {
	std::stringstream buffer;
	buffer << v.x << "," << v.y << "," << v.z;
	return buffer.str();
}

double time_now() {
    //std::chrono::high_resolution_clock::time_point rightNow =
        //std::chrono::high_resolution_clock::now();
    std::chrono::time_point<std::chrono::high_resolution_clock,std::chrono::nanoseconds> rightNow =
        std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::nano> epochTime = rightNow.time_since_epoch();
    return epochTime.count();
}
