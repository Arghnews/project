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
    p_state(mass) {

}

/*
 * Code for movement, relative locking of camera
 * Need to be able to apply forces and pass these through to p_state
 * Changes of p_state need to affect l_cub or set a flag
 * To recalc l_cub whenever moved
 */
