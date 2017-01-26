#ifndef MY_PHYSICS
#define MY_PHYSICS
#include <chrono>
#include <string>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // vulkan range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include "Thing.hpp"

class World;
struct Derivative;

class Physics {
	public:
		void integrate( const std::string thingName, double, double);
		Physics(World*);
	private:
		World* world;
		Derivative evaluate( const std::string initial, double, double, const Derivative&);
		glm::vec3 world_force( Thing& state, double dt );
};
int the_main();

struct Derivative
{
    glm::vec3 dx;      // dx/dt = velocity
    glm::vec3 dp;      // dp/dt = force (change in momentum)
    Derivative () :
        dx(glm::vec3(0.0f,0.0f,0.0f)),
        dp(glm::vec3(0.0f,0.0f,0.0f))
    {
    }
};

#endif
