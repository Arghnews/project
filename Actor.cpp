#include "Actor.hpp"

Actor::Actor(
    const fv* vertexData,
    std::string vertShader,
    std::string fragShader,
    v3 topCenter,
    v3 scale,
    v3 startPos,
    float mass,
    float inertia,
    bool selectable,
    bool mobile
    ) :
    camera(),
    l_cuboid(vertexData, topCenter, scale, startPos),
    g_cuboid(vertexData, vertShader, fragShader),
    p_state(mass, inertia, startPos),
    changed_state(false),
    invisible_(false),
    selectable(selectable),
    mobile(mobile)
    {
    }

void Actor::invis(const bool& b) {
    invisible_ = b;
}

void Actor::reorient() {
    p_state.orient = fq();
    p_state.recalc();
}

const bool Actor::invis() const {
    return invisible_;
}

m4 Actor::modelMatrix() const {
    return p_state.modelMatrix(l_cuboid.scale);
}

m4 Actor::viewMatrix() const {
    return p_state.viewMatrix(l_cuboid.scale);
}

const P_State& Actor::get_state() const {
    return p_state;
}

const L_Cuboid& Actor::logical_cuboid() {
    if (changed_state) {
        l_cuboid.recalc(p_state.position,p_state.orient);
        changed_state = false;
    }
    return l_cuboid;
}

const G_Cuboid& Actor::graphical_cuboid() const {
    return g_cuboid;
}

P_State& Actor::state_to_change() {
    changed_state = true;
    return p_state;
}

void Actor::apply_force(const Force& force) {
    p_state.apply_force(force);
}

/*
 * Code for movement, relative locking of camera
 * Need to be able to apply forces and pass these through to p_state
 * Changes of p_state need to affect l_cub or set a flag
 * To recalc l_cub whenever moved
 */
