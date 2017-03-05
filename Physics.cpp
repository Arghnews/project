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
#include "Force.hpp"
#include <cmath>
#include <algorithm>

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
    output.dL = simple_torque_resolve(state, dt);
    return output;
}

v3 Physics::simple_torque_resolve(const P_State& state, float dt) {
    v3 net_affected(zeroV);
    v3 net_unaffected(zeroV);

    // force delta is half of forces on object * delta time
    //net += state.net_force() * dt * 0.5f;
    for (const auto& f: state.net_forces()) {
        if (f.t == Force::Type::Torque) {
            v3 add(f.force);
            if (f.relative) {
                add = glm::normalize(state.orient) * add;
            }
            if (f.affected) {
                net_affected += add;
            } else {
                net_unaffected += add;
            }
        }
    }

    net_affected *= 0.5f;
    net_affected += -0.8f * state.ang_momentum;
    net_affected *= dt;

    net_unaffected *= 0.5f * dt;

    const v3 net = net_affected + net_unaffected;

    return net;
}

v3 Physics::simple_force_resolve(const P_State& state, float dt) {
    v3 net_affected(zeroV);
    v3 net_unaffected(zeroV);

    //net += state.net_force() * dt * 0.5f;
    //net += -0.75f * state.momentum;

    // force delta is half of forces on object * delta time
    //net += state.net_force() * dt * 0.5f;
    for (const auto& f: state.net_forces()) {
        if (f.t == Force::Type::Force) {
            v3 add(f.force);
            if (f.relative) {
                add = glm::normalize(state.orient) * add;
            }
            if (f.affected) {
                net_affected += add;
            } else {
                net_unaffected += add;
            }
        }
    }

    const v3 signs(sgn(state.momentum.x), sgn(state.momentum.y), sgn(state.momentum.z));
    net_affected += -0.2f * signs * state.momentum * state.momentum;
    net_affected += -0.1f * state.momentum;

    const float fricLim = 0.002f;
    v3 friction;
    friction = state.momentum;
    friction.x = -1.0f * signs.x * std::min(fricLim, std::fabs(friction.x));
    friction.y = -1.0f * signs.y * std::min(fricLim, std::fabs(friction.y));
    friction.z = -1.0f * signs.z * std::min(fricLim, std::fabs(friction.z));
    net_affected += friction;

    net_affected *= dt;

    net_unaffected *= dt;

    const v3 net = net_affected + net_unaffected;

    return net;
}

bool Physics::integrate(P_State& state,
        float t, 
        float dt ) {
    Derivative a,b,c,d;

    const float TINY = 0.000008f;
    const float SMALL = TINY * TINY;

    // if no external forces going to change state
    if (state.net_forces().size() == 0) {
        if (state.momentum.x * state.momentum.x < SMALL &&
            state.momentum.y * state.momentum.y < SMALL &&
            state.momentum.z * state.momentum.z < SMALL) {
            state.momentum = zeroV;
        }
        if (state.ang_momentum.x * state.ang_momentum.x < SMALL &&
            state.ang_momentum.y * state.ang_momentum.y < SMALL &&
            state.ang_momentum.z * state.ang_momentum.z < SMALL) {
            state.ang_momentum = zeroV;
        }
        if (state.momentum == zeroV && state.ang_momentum == zeroV) {
            return false;
        }
    }

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
    state.recalc(); // update secondary values
    return true;
}

void Physics::newOrient(P_State& state, const float& dt) {
    const v3 v = state.ang_velocity * dt;

    // so no roll
    const v3 framePitch(0.0f, v.y, 0.0f);
    const v3 frameYaw(v.x, 0.0f, 0.0f);
    state.orient = fq(framePitch) * state.orient * fq(frameYaw);
    state.orient = glm::normalize(state.orient);

    //fq q = fq(v);
    //state.orient = glm::normalize(0.5f * state.orient * q);
}
