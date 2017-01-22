#include "Movement.hpp"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include "State.hpp"
#include "Util.hpp"
#include <iostream>
#include <sstream>
#include <string>

State rotateShape(Id shape, const v3& rotateBy);
State setOrient(Id shape, const fq& quat);
State translateShape(Id shape, const v3& translate);

std::ostream& operator<<(std::ostream& stream, const Movement& m) {
    std::string type = "";
    if (m.t == Movement::Transform::Rotation) {
        type = "rotation";
    } else if (m.t == Movement::Transform::Translation) {
        type = "translation";
    } else if (m.t == Movement::Transform::TranslationTopCenter) {
        type = "translationTopCenter";
    }
    return stream << "Movement on " << m.shape << " of type " << type << " of " << m.state;
}

Movement::Movement(Id shape, Transform t, v3 transforma) : shape(shape), t(t) {
    if (t == Movement::Transform::Rotation) {
        state.rotation = transforma;
    } else if (t == Movement::Transform::Translation) {
        state.pos = transforma;
    } else if (t == Movement::Transform::TranslationTopCenter) {
        state.topCenter = transforma;
    }
}

Movement::Movement(Id shape, Transform t, fq orient) : shape(shape), t(t) {
    if (t == Movement::Transform::Orient) {
        state.orient = orient;
    }
}

Movement::Movement (Id shape, Transform t, State state) : shape(shape), t(t), state(state) {}

Movement::Movement(const Movement& m) : 
    state(m.state),
    t(m.t),
    shape(m.shape) {
    }

Movement& Movement::operator=(const Movement& m) {
        if (this != &m) {
            state = m.state;
            t = m.t;
            shape = m.shape;
        }
        return *this;
    }

State Movement::move() {
    if (t == Movement::Transform::Rotation) {
        return rotateShape(shape,state.rotation);
    } else if (t == Movement::Transform::Translation) {
        return translateShape(shape,state.pos);
    } else if (t == Movement::Transform::TranslationTopCenter) {
        return translateShape(shape,state.topCenter);
    } else if (t == Movement::Transform::Orient) {
        state.orient = setOrient(shape, state.orient).orient;
        return state;
    }
}

State Movement::undo() {
    if (t == Movement::Transform::Rotation) {
        return rotateShape(shape,-1.0f*state.rotation);
    } else if (t == Movement::Transform::Translation) {
        return translateShape(shape,-1.0f*state.pos);
    } else if (t == Movement::Transform::TranslationTopCenter) {
        return translateShape(shape,-1.0f*state.topCenter);
    } else if (t == Movement::Transform::Orient) {
        return setOrient(shape,state.orient);
    }
}
