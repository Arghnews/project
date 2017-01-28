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
        P_State(float m, float inertia, v3 pos=v3());
        // primary
        v3 position;
        v3 momentum;

        fq orient;
        v3 ang_momentum;

        // secondary
        v3 velocity;

        // spin = quat angular rotation
        fq spin; // 0.5*ang_velocity*orient
        v3 ang_velocity;

        const float mass;
        const float inverse_mass;
        const float inertia;
        const float inverse_inertia;

        void add_force(const v3& force); // relative to orient
        void add_force_abs(const v3& force); // absolute, ie. down
        void clear_forces();
        void recalc();
        v3 net_force() const;

        m4 positionMatrix();
        m4 viewMatrix();

        friend std::ostream& operator<<(std::ostream& stream, const P_State& state);
};

#endif
