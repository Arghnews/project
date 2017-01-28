#ifndef MY_PHYSICS
#define MY_PHYSICS
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Util.hpp"
#include "P_State.hpp"

struct Derivative;

class Physics {
	public:
		void integrate(P_State&, float t, float dt);
		Physics();
	private:
        v3 simple_force_resolve(const P_State& state, float dt);
        v3 torque(const P_State& state, float dt);
		Derivative evaluate(P_State, float t, float dt, const Derivative&);
};

struct Derivative {
    v3 dx; // dx/dt = velocity
    v3 dp; // dp/dt = force (change in momentum)
    fq spin; // 
    v3 torque;
    Derivative () {
    }
};

#endif
