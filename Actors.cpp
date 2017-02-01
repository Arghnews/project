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

Actors::Actors() {}

void Actors::apply_force(const Id& id, const Force& force) {
    actors[id]->apply_force(force);
}

Id Actors::insert(Actor* a) {
    return insert(actors.size(), a);
}

Id Actors::insert(const Id& id, Actor* a) {
    actors.insert(std::make_pair(id,a));
    return id;
}

void Actors::check() {
    if (actors.count(selected_) == 0) {
        if (actors.size() == 0) {
            std::cout << "There are no actors to switch the next to\n";
        }
        selected_ = actors.begin()->first;
    }
}

const Id Actors::selected() {
    check();
    return selected_;
}

Actor& Actors::selectedActor() {
    return *actors[selected()];
}

// gets next selectable actor
void Actors::next() {
    check();
    const int num_actors = actors.size();
    int i=0;
    do  {
        actors[selected_]->invis(false);
        auto it = actors.find(selected_);
        if (is_last(it, actors)) {
            selected_ = actors.begin()->first;
        } else {
            selected_ = (*std::next(it)).first;
        }
    }
    while (i < num_actors && selectedActor().selectable == false);
    std::cout << "Selected cube " << selected_ << "\n";
    actors[selected_]->invis(true);
}

Actor& Actors::operator[](const Id& id) {
    return *actors[id];
}

Actors::~Actors() {
    for (auto& a: actors) {
        delete a.second;
    }
}
