#ifndef ACTORS_HPP
#define ACTORS_HPP

#include <glm/glm.hpp>
#include "Util.hpp"
#include "Force.hpp"
#include "Actor.hpp"

#include <vector>
#include <iterator>
#include <map>

class Actors {
    private:
        //std::map<Id, Actor*> actors;
        std::vector<Actor> actors;
        int selected_index;
        void check();

    public:

        // because I'm too lazy to implement an iterator wrapper
        //const std::map<Id, Actor*>& underlying() const;
        std::vector<Actor>& underlying();

        Actors();
        ~Actors();

        void apply_force(const Force& force);

        bool insert(const Id& id, Actor a);

        void select(const Id& id);
        Id selected();

        Actor& selectedActor();

        template <typename Iter, typename Cont>
        bool is_last(Iter iter, const Cont& cont) {
            return (iter != cont.end()) && (std::next(iter) == cont.end());
        }

        void next();

        Actor& operator[](const Id& id);
        int size() const;

};

#endif
