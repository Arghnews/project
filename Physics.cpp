#include <iostream>
#include <math.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Util.hpp"
#include "Physics.hpp"
#include "P_State.hpp"

Physics::Physics() {
}

// passed by value
Derivative Physics::evaluate(P_State state,
        float t, 
        float dt, 
        const Derivative& d ) {
    // advance position/velocity by their change*change in time
	// need here to make deep copy of object
	state.position = state.position + d.dx*dt;
	state.momentum = state.momentum + d.dp*dt;
    // new change in position becomes new velocity
    // new change in velo becomes new state change with respect to time
    Derivative output;
    output.dx = state.velocity;
    output.dp = simple_force_resolve(state, dt);
    return output;
}

v3 Physics::simple_force_resolve(const P_State& state, float dt) {
    v3 net(zeroV);
    // force delta is half of forces on object * delta time
    net += state.net_force() * dt * 0.5f;
    return net;
}

void Physics::integrate(P_State& state,
        float t, 
        float dt ) {
    Derivative a,b,c,d;

    // essentially calc new pos/velo at time t with dt at 0
    // then on that go again with half dt, repeat this
    // then find for full dt change
    // weighted average using Taylor series
	// these don't change the statec
    a = evaluate(state, t, 0.0f, Derivative() );
    b = evaluate(state, t, dt*0.5f, a );
    c = evaluate(state, t, dt*0.5f, b );
    d = evaluate(state, t, dt, c );

    v3 dxdt = 1.0f / 6.0f * 
        ( a.dx + 2.0f*(b.dx + c.dx) + d.dx );

    v3 dpdt = 1.0f / 6.0f * 
        ( a.dp + 2.0f*(b.dp + c.dp) + d.dp );

    state.position = state.position + dxdt * dt;
    state.momentum = state.momentum + dpdt * dt;
	state.clear_forces();
}

/*
v3 Physics::world_force( Thing& state, float dt ) {
    v3 force(zeroV);
    // add object's forces on
	v3 f(state.net_force() * dt);
	force += f;

    // air resistance
    // proportional to square of velocity of object

    bool air = true;
	glm::vec3 airRes = glm::vec3(0.0f,0.0f,0.0f);
    if (air) {
		const float air_decel = -0.25f;
        airRes.x += air_decel*state.momentum.x;
        airRes.y += air_decel*state.momentum.y;
        airRes.z += air_decel*state.momentum.z;
		airRes *= (float)dt;
    }
	force += airRes;
	//std::cout << "Force after air " << printVec(force) << "\n";
    
	bool decelF = false;
    // this is a force to stop the movement really, so don't get the sliding forever
	if (decelF) {
		float minForce = 5.0f*std::fabs(state.mass);
		float decel = 1.0f;
		force.x += -signOf(state.velocity().x) *
			std::min(minForce,decel*std::fabs(state.momentum.x));  
		force.y += -signOf(state.velocity().y) *
			std::min(minForce,decel*std::fabs(state.momentum.y));  
		force.z += -signOf(state.velocity().z) *
			std::min(minForce,decel*std::fabs(state.momentum.z));  
	}
    // gravity - in my program, z is up
    // will implement later could also do friction, but not now
	//std::cout << "Net force on object " << printVec(force) << "\n";
    
    return force;
}
*/
