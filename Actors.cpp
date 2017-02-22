#include <glm/glm.hpp>

#include "Actors.hpp"
#include "Util.hpp"
#include "Force.hpp"
#include "Actor.hpp"

#include <iterator>
#include <map>

// because I'm too lazy to implement an iterator wrapper
const std::map<Id, Actor*>& Actors::underlying() const {
    return actors;
}

Actors::Actors() : selected_index(0) {}

void Actors::apply_force(const Id& id, const Force& force) {
    actors[id]->apply_force(force);
}

bool Actors::insert(const Id& id, Actor* a) {
    auto ret = actors.insert(std::make_pair(id,a));
    return ret.second;
}

void Actors::check() {
    assert(actors.size() > 0 && "Need actors to select from");
}

Id Actors::selected() {
    check();
    return actors[selected_index]->id;
}

Actor& Actors::selectedActor() {
    check();
    return *actors[selected_index];
}

// gets next selectable actor
void Actors::next() {
    check();
    bool anySelectable = false;
    for (const auto& a:actors) {
        if (a.second->selectable == true) {
            anySelectable = true;
            break;
        }
    }
    if (!anySelectable) {
        std::cout << "Cannot select next actor, none selectable\n";
        return;
    }
    do {
        ++selected_index;
        selected_index %= actors.size();
    } while (selectedActor().selectable == false);
}

Actor& Actors::operator[](const Id& id) {
    return *actors[id];
}

int Actors::size() const {
    return actors.size();
}

Actors::~Actors() {
    for (auto& a: actors) {
        delete a.second;
    }
}
