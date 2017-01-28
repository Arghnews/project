#include "Actor.hpp"

Actor::Actor(
    const fv* vertexData,
    std::string vertShader,
    std::string fragShader,
    v3 topCenter,
    float mass
    ) :
    camera(),
    l_cuboid(vertexData, topCenter),
    g_cuboid(vertexData, vertShader, fragShader),
    p_state(mass),
    changed_state(false)
    {

}

const L_Cuboid& Actor::logical_cuboid() {
    if (changed_state) {
        l_cuboid.recalc();
        changed_state = false;
    }
    return l_cuboid;
}

P_State& Actor::state_to_change() {
    changed_state = true;
    return p_state;
}

void Actor::add_force(const v3& force) {
    p_state.add_force(force);
}

void Actor::add_force_abs(const v3& force) {
    p_state.add_force_abs(force);
}

/*
 * Code for movement, relative locking of camera
 * Need to be able to apply forces and pass these through to p_state
 * Changes of p_state need to affect l_cub or set a flag
 * To recalc l_cub whenever moved
 */
