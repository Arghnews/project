#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include "cereal/archives/binary.hpp"

namespace cereal {
    template<class Archive>
        void serialize(Archive& archive, glm::vec3& v) {
            archive(v.x, v.y, v.z);
        }
}

struct Classy {
   int x;
   glm::vec3 y;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(x, y);
    }
};

int main() {
    std::cout << "Mainly\n";

    std::stringstream ss; // any stream can be used

    {
        cereal::BinaryOutputArchive oarchive(ss); // Create an output archive
        Classy c1_in;
        c1_in.x = 12.0f;
        c1_in.y.x = 69.0f;
        oarchive(c1_in); // Write the data to the archive
    } // archive goes out of scope, ensuring all contents are flushed

    // insert networked magic here
    
    Classy c2_in;
    {
        cereal::BinaryInputArchive iarchive(ss); // Create an input archive
        iarchive(c2_in); // Read the data from the archive
    }
    std::cout << c2_in.x << "," << c2_in.y.x << "\n";

}
