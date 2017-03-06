#ifndef SHOT_HPP
#define SHOT_HPP

struct Shot {
    Id shooter;
    Shot() : Shot(-1) {}
    Shot(const Id& shooter) : shooter(shooter), hit(false) {}
    bool hit;
    Id target;
    v3 org; // where id was when fired
    v3 dir; // id's direction of shot
    v3 hit_pos; // world coord of the shot's position
};

#endif
