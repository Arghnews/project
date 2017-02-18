#include <glm/glm.hpp>

#include "Octree.hpp"
#include "AABB.hpp"

#include "Util.hpp"

#include <thread>
#include <iostream>
#include <set>
#include <algorithm>
#include <cstdlib>

int main() {
    vv3Id posIds;
    static const float areaSize = 10000.0f;
    static const float seperator = 1.03f;
    static const float floor_mass = 1.0f;
    static const float scaleFactor = 1.0f;
    int id = 0;
    const int n = 1000;
    Octree tree(zeroV, areaSize);
    for (int i=0; i<n; ++i) {
            const v3 position(v3(i,i,i));
            tree.insert(position,id);
            posIds.push_back(std::make_pair(position,id));
            ++id;
    }
    for (auto& p: posIds) {
        std::cout << "Pair " << p.second << "," << printV(p.first) << "\n";
    }
    static long fps_max = 60l;
    int frame = 0;
    long currentTime = timeNowMicros();
    srand (static_cast <unsigned> (timeNowMicros()));
    while (true) {

        long newTime = timeNowMicros();
        //std::cout << "TIme start " << newTime << "\n";
        long frameTime = newTime - currentTime;
        currentTime = newTime;
        std::cout << "frame:" << frame++ << "\n";
        // sleep if fps would be > fps_max

        // move them
        // /*
        static bool b = false;
        const v3 c(1.0f,1.0f,1.0f);
        for (auto& p: posIds) {
            const bool deleted = tree.del(p.first, p.second);
            assert(deleted && "Should always be able to delete last cube position");
            float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            if (b) {
                p.first += c;
            } else {
                p.first += -c;
            }
            tree.insert(p.first, p.second);
            //std::cout << "Pair " << p.second << "," << printV(p.first) << "\n";
        }
        b = !b;
        //1.73205
        // retrieve them
        //
        // do stuff

        //std::cout << "Frametime " << frameTime << "\n";
        //std::cout << "TIme now " << timeNowMicros() << "\n";
        long diff = timeNowMicros() - newTime;
        std::cout << "Diff " << diff << "\n";
        //long spareFrameTime = (1e6l / fps_max) - (timeNowMicros() - newTime);
        if (diff > 10000l) {
            //std::cout << "---\t---- Spare frame time " << spareFrameTime << "---\t---\n";
        }
        //std::this_thread::sleep_for(std::chrono::microseconds(std::max(0l,spareFrameTime)));
    }
    std::cout << "asd\n";
}
