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
#include "Force.hpp"

#include <string>

class Actor {
    private:
        Camera camera;
        L_Cuboid l_cuboid;
        G_Cuboid g_cuboid;
        P_State p_state;
        bool changed_state;
        bool invisible_;
    public:
        bool selectable;
        bool mobile;
        const v3 scale;
        void invis(const bool& b);
        const bool invis() const;
        const P_State& get_state() const;
        const L_Cuboid& logical_cuboid();
        const G_Cuboid& graphical_cuboid() const;
        P_State& state_to_change();
        void reorient();
        m4 viewMatrix() const;
        m4 modelMatrix() const;
        void apply_force(const Force& force);
        Actor(const fv* vertexData,
        std::string vertShader,
        std::string fragShader,
        v3 topCenter,
        v3 scale,
        v3 startPos,
        float mass,
        float inertia,
        bool selectable,
        bool mobile);
};

#endif
