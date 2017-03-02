#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include "cereal/archives/binary.hpp"
#include "cereal/details/traits.hpp"

struct Classy {
    int x;
    int y;

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
        Classy c1;
        c1.x = 324;
        oarchive(c1); // Write the data to the archive
    } // archive goes out of scope, ensuring all contents are flushed

    {
        cereal::BinaryInputArchive iarchive(ss); // Create an input archive
        Classy c1_in;
        iarchive(c1_in); // Read the data from the archive
        std::cout << c1_in.x << "\n";
    }

}
