#ifndef ACTORS_HPP
#define ACTORS_HPP

#include <glm/glm.hpp>
#include "Util.hpp"
#include "Actor.hpp"

#include <iterator>
#include <map>

class Actors : std::map<Id, Actor*> {
    private:
        std::map<Id, Actor*> actors;
        Id selected_;
        void check();

    public:

        // because I'm too lazy to implement an iterator wrapper
        const std::map<Id, Actor*>& underlying() const;

        Actors();
        ~Actors();

        void apply_force(const Id& id, const v3& force);
        void apply_force(const Id& id, const v3& force, const v3& point);

        void apply_torque(const Id& id, const v3& force);

        Id insert(Actor* a);

        Id insert(const Id& id, Actor* a);

        const Id selected();

        Actor& selectedActor();

        template <typename Iter, typename Cont>
        bool is_last(Iter iter, const Cont& cont) {
            return (iter != cont.end()) && (std::next(iter) == cont.end());
        }

        void next();

        Actor& operator[](const Id& id);

};

#endif
