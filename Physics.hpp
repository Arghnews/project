#ifndef MY_PHYSICS
#define MY_PHYSICS
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Util.hpp"
#include "Force.hpp"
#include "P_State.hpp"

struct Derivative;

class Physics {
	public:
        // true if changed, false if not
		bool integrate(P_State&, float t, float dt);
		Physics();
	private:
        void newOrient(P_State& state, const float& dt);
        v3 simple_force_resolve(const P_State& state, float dt);
        v3 simple_torque_resolve(const P_State& state, float dt);
		Derivative evaluate(P_State, float t, float dt, const Derivative&);
};

struct Derivative {
    v3 dx; // dx/dt = velocity
    v3 dp; // dp/dt = force (change in momentum)
    v3 dL; // dL/dt = change in angular momentum
    Derivative () {
    }
};

#endif
