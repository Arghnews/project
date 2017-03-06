#ifndef ARCHIVER_HPP
#define ARCHIVER_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cereal/types/deque.hpp>
#include "cereal/archives/portable_binary.hpp"
#include "Util.hpp"
#include "Force.hpp"

// Portable binary checks endianness etc, for small overhead

namespace cereal {
    template<class Archive>
        void serialize(Archive& archive, glm::vec3& v) {
            archive(v.x, v.y, v.z);
        }
    template<class Archive>
        void serialize(Archive& archive, Force& f) {
            archive(f.id, f.force, f.t, f.relative, f.affected);       
        }
}


#endif
