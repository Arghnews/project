#ifndef ARCHIVER_HPP
#define ARCHIVER_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/vector.hpp"
#include <cereal/types/deque.hpp>
#include "Util.hpp"
#include <algorithm>
#include <cmath>

// Portable binary checks endianness etc, for small overhead

namespace cereal {
    template<class Archive>
        void serialize(Archive& archive, v3& v) {
            archive(v.x, v.y, v.z);
        }
    template<class Archive>
        void serialize(Archive& archive, fq& q) {
            archive(q.w, q.x, q.y, q.z);
        }
    /*
    template<class Archive>
        void save(Archive& archive, fq const& q) {
            assert(1.0f-std::fabs(q.w) < 0.05f);
            archive(q.x, q.y, q.z);
        }
    template<class Archive>
        void load(Archive& archive, fq& q) {
            archive(q.x, q.y, q.z);
            // x^2+y^2+z^2+w^2 = 1, for len 1 quat
            // 1 - (x^2+y^2+z^2) = w^2
            // w = sqrt(1 - (x^2+y^2+z^2))
            q.w = sqrt(1.0f - (q.x*q.x + q.y*q.y + q.z*q.z));
        }
        */
}

#endif
