#ifndef MY_HELPER_FILE
#define MY_HELPER_FILE

#include <string>
#include <chrono>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // vulkan range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

double time_now();
typedef std::chrono::time_point<std::chrono::high_resolution_clock,std::chrono::nanoseconds> timey;

template <typename T> T signOf(T val) {
	return (T(0) < val) - (val < T(0));
}
std::string printVec(glm::vec3 v);
#endif
