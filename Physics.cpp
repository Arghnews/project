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
    newOrient(state, dt);
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
    v3 net(zeroV);
    net += state.net_torque() * dt * 0.5f;

    net += -0.75f * state.ang_momentum;
    return net;
}

v3 Physics::simple_force_resolve(const P_State& state, float dt) {
    v3 net(zeroV);
    // force delta is half of forces on object * delta time
    net += state.net_force() * dt * 0.5f;
    
    net += -0.75f * state.momentum;
    std::cout << "dt:" << dt << " Net force on object " << printV(net) << "\n";
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
    
    //state.orient = 0.5f * fq(state.ang_velocity * dt) * state.orient;
    newOrient(state, dt);

	state.clear_forces(); // clear forces vector
	state.clear_torques(); // clear torques vector
    state.recalc(); // update secondary values
}

void Physics::newOrient(P_State& state, const float& dt) {
    const v3 v = state.ang_velocity * dt;
    fq q = fq(v);
    //fq q(1.0f, v.x, v.y, v.z);
    state.orient = 0.5f * state.orient * q;
}
