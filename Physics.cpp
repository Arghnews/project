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
    state.ang_momentum = state.ang_momentum + d.dL * dt;
    fq q(state.ang_velocity * dt);
    q.w = 0.0f;
    state.orient = 0.5f * q * state.orient;
    // new change in position becomes new velocity
    // new change in velo becomes new state change with respect to time
    Derivative output;
    output.dx = state.velocity;
    output.dp = simple_force_resolve(state, dt);
    output.dq = state.spin;
    output.dL = simple_torque_resolve(state, dt);
    return output;
}

v3 Physics::simple_torque_resolve(const P_State& state, float dt) {
    // small torque rotating about x
    //return v3(1.0f,0.0f,0.0f) - state.ang_velocity * 0.1f;
    return v3(zeroV);
}

v3 Physics::simple_force_resolve(const P_State& state, float dt) {
    v3 net(zeroV);
    // force delta is half of forces on object * delta time
    net += state.net_force() * dt * 0.5f;
    
    net += -0.5f * state.velocity;
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

    v3 dLdt = 1.0f / 6.0f * 
        ( a.dL + 2.0f*(b.dL + c.dL) + d.dL );

    state.position = state.position + dxdt * dt;
    state.momentum = state.momentum + dpdt * dt;
    state.ang_momentum = state.ang_momentum + dLdt * dt;
    fq q(state.ang_velocity * dt);
    q.w = 0.0f;
    state.orient = 0.5f * q * state.orient;

	state.clear_forces(); // clear forces vector
    state.recalc(); // update secondary values
}
