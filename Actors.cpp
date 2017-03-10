#include <glm/glm.hpp>

#include "Actors.hpp"
#include "Util.hpp"
#include "Force.hpp"
#include "Actor.hpp"

#include <iterator>
#include <vector>

// because I'm too lazy to implement an iterator wrapper
std::vector<Actor>& Actors::underlying() {
    return actors;
}

Actors::Actors() : selected_index(0) {}

void Actors::apply_force(const Force& force) {
    actors[force.id].apply_force(force);
}

bool Actors::insert(const Id& id, Actor a) {
    //auto ret = actors.insert(std::make_pair(id,a));
    //return ret.second;
    auto ret = actors.size();
    actors.emplace_back(a);
    return id == ret;
}

void Actors::check() {
    assert(actors.size() > 0 && "Need actors to select from");
}

Id Actors::selected() {
    check();
    actors[selected_index].invis(true);
    return actors[selected_index].id;
}

Actor& Actors::selectedActor() {
    check();
    actors[selected_index].invis(true);
    return actors[selected_index];
}

// gets next selectable actor
void Actors::next() {
    check();
    actors[selected_index].invis(false);
    bool anySelectable = false;
    for (const auto& a:actors) {
        if (a.selectable == true) {
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
    for (auto& a:actors) {
        a.invis(false);
    }
    actors[selected_index].invis(true);
}

Actor& Actors::operator[](const Id& id) {
    return actors[id];
}

int Actors::size() const {
    return actors.size();
}

Actors::~Actors() {
    /*
    for (auto& a: actors) {
        delete a.second;
    }*/
}

/*
// because I'm too lazy to implement an iterator wrapper
const std::map<Id, Actor*>& Actors::underlying() const {
    return actors;
}

Actors::Actors() : selected_index(0) {}

void Actors::apply_force(const Force& force) {
    actors[force.id]->apply_force(force);
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
    actors[selected_index]->invis(true);
    return actors[selected_index]->id;
}

Actor& Actors::selectedActor() {
    check();
    actors[selected_index]->invis(true);
    return *actors[selected_index];
}

// gets next selectable actor
void Actors::next() {
    check();
    actors[selected_index]->invis(false);
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
    for (const auto& a:actors) {
        a.second->invis(false);
    }
    actors[selected_index]->invis(true);
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
*/
