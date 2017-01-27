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
        P_State(float m, v3 pos=v3());
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

        void add_force(const v3& force); // relative to orient
        void add_force_abs(const v3& force); // absolute, ie. down
        void clear_forces();
        void recalculate();
        v3 net_force() const;

        void turn(const v3& v); // turn by vec, ie. left

        friend std::ostream& operator<<(std::ostream& stream, const P_State& state);
};

#endif
