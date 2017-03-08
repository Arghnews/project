#ifndef MTV_HPP
#define MTV_HPP

#include <glm/glm.hpp>

#include <iostream>
#include <sstream>

#include "Util.hpp"

struct MTV {
    v3 axis;
    float overlap;
    Id id1;
    Id id2;
    bool colliding;
    friend bool operator<(const MTV& m1, const MTV& m2) {
        //return m1.id1 < m2.id1 && m1.id2 < m2.id2;
        return std::make_pair(m1.id1, m1.id2) < std::make_pair(m2.id1, m2.id2);;
    }
    friend std::ostream& operator<<(std::ostream& os, const MTV& m) {
        return os << "(" << m.id1 << "," << m.id2 << ")";
    }
};

#endif
