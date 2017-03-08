#include "Actor.hpp"

int Actor::num_actors = 0;

Actor::Actor(
    int g_cub,
    const vv3* face_verts,
    v3 scale,
    v3 startPos,
    float mass,
    float inertia,
    bool selectable,
    bool mobile
    ) :
    l_cuboid(face_verts, scale, startPos),
    g_cuboid(g_cub),
    p_state_(mass, inertia, startPos),
    changed_state_(false),
    invisible_(false),
    selectable(selectable),
    mobile(mobile),
    id(num_actors++)
    {
    }

void Actor::invis(const bool& b) {
    invisible_ = b;
}

const bool Actor::invis() const {
    return invisible_;
}

m4 Actor::modelMatrix() const {
    return p_state_.modelMatrix(l_cuboid.scale);
}

m4 Actor::viewMatrix() const {
    return p_state_.viewMatrix(l_cuboid.scale);
}

const P_State& Actor::p_state() const {
    return p_state_;
}

const L_Cuboid& Actor::logical_cuboid() const {
    return l_cuboid;
}

int Actor::graphical_cuboid() const {
    return g_cuboid;
}

void Actor::set_changed() {
    changed_state_ = true;
}

void Actor::recalc() {
    static const std::string s = "Should only ever recalc an actor's state if it changed "
    "- if strange bug here after time, take care that physics sim clamps mom/ang_mom to zero, "
    " try removing clamping";
    assert((s,changed_state_));
    l_cuboid.recalc(p_state_.position,p_state_.orient);
    changed_state_ = false;
}

bool Actor::changed_state() const {
    return changed_state_;
}

P_State& Actor::state_to_change() {
    return p_state_;
}

void Actor::apply_force(const Force& force) {
    if (mobile) {
        p_state_.apply_force(force);
    }
}

/*
 * Code for movement, relative locking of camera
 * Need to be able to apply forces and pass these through to p_state_
 * Changes of p_state_ need to affect l_cub or set a flag
 * To recalc l_cub whenever moved
 */
