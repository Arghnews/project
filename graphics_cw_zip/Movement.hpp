#ifndef MOVEMENT_H
#define MOVEMENT_H

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include "State.hpp"
#include "Util.hpp"
#include <iostream>
#include <sstream>

struct Movement {
    enum Transform { Rotation, Translation, TranslationTopCenter, Orient };
    State state; // if all aligned doesn't matter
    Transform t; // biggest first for better packing (maybe)
    Id shape;
    friend std::ostream& operator<<(std::ostream& stream, const Movement& m);
    Movement(Id shape, Transform t, v3 transforma);
    Movement(Id shape, Transform t, fq orient);
    Movement (Id shape, Transform t, State state);
    Movement (Id shape, Transform t, State before, State after);
    Movement(const Movement& m);
    Movement& operator=(const Movement& m);
    State move();
    State undo();
};

#endif
