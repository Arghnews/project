#ifndef ARCHIVER_HPP
#define ARCHIVER_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/vector.hpp"
#include <cereal/types/deque.hpp>
#include "Util.hpp"

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
}

#endif
