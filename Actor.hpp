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
#include "Force.hpp"

#include <string>

class Actor {
    private:
        L_Cuboid l_cuboid;
        int g_cuboid;
        int l_cub_face_vert;
        P_State p_state_;
        bool changed_state_;
        bool invisible_;
    public:
        static int num_actors;
        Id id;
        bool selectable;
        bool mobile;
        const v3 scale;
        void invis(const bool& b);
        const bool invis() const;

        void set_changed();
        void set_unchanged();
        bool changed_state() const;
        void recalc();
        const P_State& p_state() const;
        P_State& state_to_change();
        const L_Cuboid& logical_cuboid() const;

        int graphical_cuboid() const;
        m4 viewMatrix() const;
        m4 modelMatrix() const;
        void apply_force(const Force& force);
        Actor(
        int g_cub,
        const vv3* face_verts,
        v3 scale,
        v3 startPos,
        float mass,
        float inertia,
        bool selectable,
        bool mobile
        );
};

#endif
