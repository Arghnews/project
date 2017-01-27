#ifndef ACTOR_HPP
#define ACTOR_HPP

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Util.hpp"
#include "P_State.hpp"
#include "G_Cuboid.hpp"
#include "L_Cuboid.hpp"
#include "Camera.hpp"

#include <string>

class Actor {
    private:
        Camera camera;
        L_Cuboid l_cuboid;
        G_Cuboid g_cuboid;
        P_State p_state;
    public:
        Actor(const fv* vertexData,
        std::string vertShader,
        std::string fragShader,
        v3 topCenter,
        float mass);
};

#endif
