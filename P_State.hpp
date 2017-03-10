#ifndef P_STATE_HPP
#define P_STATE_HPP

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Force.hpp"
#include "Util.hpp"

#include <iosfwd>

class P_State {
    private:
        Forces forces;
    public:
        // just here so that serialize can do it's thing
        P_State();
        P_State(float m, float inertia, v3 pos, Id id);
        Id id;
        // primary
        v3 position;
        v3 momentum;

        fq orient;
        v3 ang_momentum;

        // secondary
        v3 velocity;

        v3 ang_velocity;

        const float mass;
        const float inverse_mass;
        const float inertia;
        const float inverse_inertia;

        void set_momentum(const v3& mom);

        void apply_force(const Force& force);
        void clear_forces();
        void recalc();
        const Forces& net_forces() const;
        v3 facing() const;

        m4 modelMatrix(const v3& scale) const;
        m4 viewMatrix(const v3& scale) const;

        friend std::ostream& operator<<(std::ostream& stream, const P_State& state);

        // please note that when serialized the mass/inertia are not encoded
        // intended use is to set the object's state according to the serialised
        // object's state, not replace the object with the serialised one
        template<class Archive>
            void serialize(Archive& archive) {
                archive(position, momentum, orient, ang_momentum, id);       
            }
};

#endif
