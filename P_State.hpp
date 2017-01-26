#ifndef P_STATE_HPP
#define P_STATE_HPP

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Util.hpp"

#include <iostream>
#include <sstream>

class P_State {
    private:
        vv3 forces;
    public:
        P_State(v3 pos, float m);
        // primary
        v3 position;
        v3 momentum;
        v3 force;
        fq orient;

        // secondary
        v3 velocity;
        v3 acceleration;

        const float mass;
        const float inverse_mass;

        void add_force(const v3& force);
        void clear_forces();
        void recalculate();
        v3 net_force() const;

        friend std::ostream& operator<<(std::ostream& stream, const P_State& state);
};

#endif
