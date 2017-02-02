#ifndef MTV_HPP
#define MTV_HPP

#include <glm/glm.hpp>

#include "Util.hpp"

struct MTV {
    v3 axis;
    float overlap;
    bool colliding;
    Id id1;
    Id id2;
    friend bool operator<(const MTV& m1, const MTV& m2) {
        return m1.id1 < m2.id1 && m1.id2 < m2.id2;
    }
};

#endif
