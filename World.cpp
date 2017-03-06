#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <map>
#include <utility>
#include <set>
#include <limits>

#include "Actors.hpp"
#include "Actor.hpp"
#include "Physics.hpp"
#include "G_Cuboid.hpp"
#include "L_Cuboid.hpp"
#include "P_State.hpp"
#include "Physics.hpp"
#include "Octree.hpp"
#include "AABB.hpp"
#include "Force.hpp"
#include "Util.hpp"
#include <cmath>
#include <algorithm>
#include <deque>

#include "MTV.hpp"
#include "World.hpp"

World::World(float worldSize, v2 windowSize, float restitution) :
    tree_(zeroV,worldSize),
    windowSize(windowSize),
    restitution(restitution) {
}

Actors& World::actors() {
    return actors_;
}

void World::insert(Actor* a) {
    const Id id = a->id;
    tree_.insert(a->p_state().position,id);
    actors_.insert(id, a);
}

Hit World::hit_face(const vv3& verts24, const v3& org, const v3& dir, const int& i) {
    Hit hit;
    const auto plane_v1 = verts24[i+1] - verts24[i+0];
    const auto plane_v2 = verts24[i+2] - verts24[i+0];
    const auto n = glm::cross(plane_v1,plane_v2);
    //const float D = glm::dot(n, verts24[i+0]);
    // plane Ax + By + Cz + D = 0
    // have D, ABC = normal
    //float t = - ( (d + glm::dot(n,org)) / glm::dot(n,dir));
    const float upper = glm::dot(verts24[i+0] - org,n);
    const float lower = glm::dot(dir,n);
    if (isZero(lower) || isZero(upper)) {
        // strictly speaking there is a case where the line is parallel to the plane
        // this seems so unlikely to occur that for now haven't bothered to accept
        // this case
        //std::cout << "No intersection (nan)\n";
    } else {
        const float d = upper / lower;
        if (d < 0.0f) {
            // face hit is behind - shooting out your bum
        } else {
            const v3 M = d * dir + org;
            //std::cout << "Intersection at (d:" << d << ") " << printV(M) << "\n";
            const v3& A = verts24[i+0];
            const v3& B = verts24[i+1];
            //const v3& C = verts24[i+2];
            const v3& D = verts24[i+3];
            const v3 AM = M - A;
            const v3 AB = B - A;
            const v3 AD = D - A;
            const float AMAB = glm::dot(AM,AB);
            const float AMAD = glm::dot(AM,AD);
            const float ABAB = glm::dot(AB,AB);
            const float ADAD = glm::dot(AD,AD);
            bool proj_AB = 0.0f <= AMAB && AMAB <= ABAB;
            bool proj_AD = 0.0f <= AMAD && AMAD <= ADAD;
            // intersect at intersection -> true
            if (proj_AB && proj_AD) {
                hit.hit = true;
                hit.pos = M;
                //std::cout << "Hit at " << printV(M) << "\n";
                // inside the rectangle
            } else {
                // intersects with plane outside face
                //std::cout << "Miss at " << printV(M) << "\n";
            }
        }
    }
    return hit;
}

Hit World::hit_actor(const v3& org, const v3& dir, const Id& id) {
    const auto& l_cub = actors_[id].logical_cuboid();
    const vv3& verts24 = l_cub.verts24;
    const int size = verts24.size();
    assert(size == 24);

    // https://en.wikipedia.org/wiki/Line%E2%80%93plane_intersection

    Hit closest;

    // test each face for hits
    // now need to try and not test against every other shape
    // first try to use octree to get blocks in front?
    // then compare dist of nearest hit
    float dist(std::numeric_limits<float>::max());
    for (int i=0; i<size; i+=4) {
        const Hit hit = hit_face(verts24, org, dir, i);
        const v3& hit_pos = hit.pos;
        if (hit.hit) {
            const v3 diff = hit_pos - org;
            const float dist_away = glm::dot(diff,diff);
            if (dist_away < dist) {
                // find closest point
                dist = dist_away;
                closest = hit;
            }
        }
    }
    // face func doesn't care about id of actor hit
    closest.id = id;
    return closest;
}

void World::firedShot(const Id& id) {
    const v3 org = actors_[id].p_state().position;
    const v3 dir = glm::normalize(actors_[id].p_state().facing());
    
    long now = timeNowMicros();
    // for now just checking against every other actor, seems to take only 1.5ms max, sometimes 0.5ms
    
    float dist(std::numeric_limits<float>::max());
    Hit closest;

    for (const auto& a: actors_.underlying()) {
        const Id& a_id = a.first;
        Actor& actor = *a.second;
        const auto& l_cub = actor.logical_cuboid();
        if (id == a_id) {
            continue;
        }
        // early exit if could not possibly be closer than dist
        // basically max diagonal distance across shape
        const float max_width = l_cub.furthestVertex * 0.5f;
        const v3 diff = actor.p_state().position - org;
        // closest possible corner/edge/face/point on shape
        const float nearest_dist = glm::dot(diff,diff) - max_width*max_width;
        if (nearest_dist > dist) {
            // no need to bother
            // even at closest possible hit could not be closer than "closest"
            continue;
        }

        Hit h = hit_actor(org, dir, a_id);
        if (h.hit) {
            const v3 diff = h.pos - org;
            const float dist_away = glm::dot(diff,diff);
            if (dist_away < dist) {
                dist = dist_away;
                closest = h;
            }
        }
    }
    
    if (closest.hit) {
        std::cout << id << " fired and hit " << closest.id << " at " << printV(closest.pos) << "\n";
    } else {
        std::cout << id << " fired and missed" << "\n";
    }

    long taken = timeNowMicros() - now;
    //std::cout << "Hit checking took " << (double)taken/1000.0 << "ms\n";
    
}

void World::simulate(const float& t, const float& dt) {
    // whenever move need to update octree
    for (const auto& a: actors_.underlying()) {
        const Id& id = a.first;
        Actor& actor = *a.second;
        // don't similate immobile objects
        if (!a.second->mobile) {
            continue;
        }
        P_State& cube_phys = (*a.second).state_to_change();
        const bool deleted = tree_.del(cube_phys.position);
        assert(deleted && "Should always be able to delete last cube position");
        const bool changed = phys_.integrate(cube_phys, t, dt);
        if (changed) {
            actor.set_changed();
            actor.recalc();
        }
        tree_.insert(cube_phys.position, id);
    }
}

void World::blow_up(const Id& id) {
    const auto collidingInto = colliding_with(id);
    for (const auto& a_id: collidingInto) {
        // push others away
    }
}

std::vector<MTV> World::colliding_with(const Id& id) {
    std::vector<MTV> collidingWith;
    const Actor& actor = actors_[id];
    const L_Cuboid& l_cub = actor.logical_cuboid();
    const P_State& phys = actor.p_state();
    const vId nearby = tree_.queryRange(phys.position, l_cub.furthestVertex);
    for (const auto& id_nearby: nearby) {
        // collision with self
        if (id_nearby == id) {
            continue;
        }
        const Actor& actor_nearby = actors_[id_nearby];
        // both actors immobile - ie. worldly blocks that don't move
        if (!actor.mobile && !actor_nearby.mobile) {
            continue;
        }
        const L_Cuboid& l_cub_nearby = actor_nearby.logical_cuboid();
        MTV mtv = L_Cuboid::colliding(l_cub,l_cub_nearby);
        const bool areColliding = mtv.colliding;
        // should move above collision check
        if (areColliding) {
            collidingWith.emplace_back(mtv);
        }
    }
    return collidingWith;
}

void World::collisions() {

    std::set<MTV> collidingPairs; // stuff that just started colliding
    //static std::map<MTV,int> alreadyColliding; // stuff that was already c
    static std::map<MTV,int> alreadyColliding; // stuff that was already colliding, for how long (ticks)
    std::set<std::pair<Id, Id>> pairsThisPass;

    long t_earlier = timeNowMicros();
    long a_total = 0l;
    long b_total = 0l;
    long c_total = 0l;
    for (const auto& a: actors_.underlying()) {
        const Id& id = a.first;
        const Actor& actor = *a.second;
        const L_Cuboid& l_cub = actor.logical_cuboid();
        const P_State& phys = actor.p_state();
        long a_start = timeNowMicros();
        const vId nearby = tree_.queryRange(phys.position, l_cub.furthestVertex);
        a_total += timeNowMicros() - a_start;
        int nearbys = 0;
        long b_start = timeNowMicros();
        for (const auto& b: nearby) {
            const Id& id_nearby = b;
            // collision with self
            if (id_nearby == id) {
                continue;
            }
            Actor& actor_nearby = actors_[id_nearby];
            // both actors immobile - ie. worldly blocks that don't move
            if (!actor.mobile && !actor_nearby.mobile) {
                continue;
            }
            ++nearbys;
            const L_Cuboid& l_cub_nearby = actor_nearby.logical_cuboid();
            long c_start = timeNowMicros();
            MTV mtv = L_Cuboid::colliding(l_cub,l_cub_nearby);
            c_total += timeNowMicros() - c_start;
            const bool areColliding = mtv.colliding;
            mtv.id1 = std::min(id, id_nearby);
            mtv.id2 = std::max(id, id_nearby);
            auto paired = std::make_pair(mtv.id1,mtv.id2);
            // should move above collision check
            if (contains(pairsThisPass, paired)) {
                continue;
            } else {
                pairsThisPass.insert(paired);
            }
            if (areColliding) {
                // if not already colliding
                // add to justColliding
                collidingPairs.insert(mtv);
                //std::coutout << "Now colliding inserting" << mtv << "\n";
            } else {
                // if not colliding, erase from already colliding set
                if (contains(alreadyColliding, mtv)) {
                    auto size = alreadyColliding.size();
                    alreadyColliding.erase(mtv);
                    //std::coutout << "No longer colliding deleting " << mtv << "\n";
                    assert(alreadyColliding.size() + 1 == size);
                }
            }
        }
        b_total += timeNowMicros() - b_start;
    }
    auto a_time = ((double)a_total)/1000.0;
    auto c_time = ((double)c_total)/1000.0;
    //std::cout << "Total time of a - octree lookup " << a_time << "ms" << "\n";
    //std::cout << "Total time of b " << ((double)b_total)/1000.0 << "ms" << "\n";
    //std::cout << "Total time of c - colliding calc " << c_time << "ms" << "\n";
    long taken = timeNowMicros() - t_earlier;
    auto time_first_bit = (double)taken/1000.0;
    //std::cout << "Other time " << time_first_bit - a_time - c_time << "ms\n";
    //std::cout << "Time taken " << time_first_bit << "ms for finding collisions" << "\n";

    std::map<Id,std::vector<Force>> forceQueue;

    taken = timeNowMicros();
    for (const auto& mtv_original: collidingPairs) {
        MTV mtv = mtv_original;

        mtv.axis = glm::normalize(mtv.axis);
        const Id& id1 = mtv.id1;
        const Id& id2 = mtv.id2;
        Actor& a1 = actors_[id1];
        Actor& a2 = actors_[id2];
        const P_State& p1 = a1.p_state();
        const P_State& p2 = a2.p_state();

        //std::coutout << id1 << " colliding with " << id2 << "\n";

        // -------------------------------------------------------- READ ME :
        // maybe consider case just using momentum resolve and solving sticking

        // resolve collision
        ////std::cout << id1 << " and " << id2 << " colliding\n";

        const float m1 = p1.mass;
        const float m2 = p2.mass;
        const v3 u1 = p1.velocity;
        const v3 u2 = p2.velocity;
        const float CR = 0.95f; // coef of restitution
        // honestly just don't change CR

        const v3 v1 = (m1*u1 + m2*u2 + m2*CR*(u2-u1)) / (m1+m2);
        const v3 v2 = (m1*u1 + m2*u2 + m1*CR*(u1-u2)) / (m1+m2);

        P_State& p_1 = a1.state_to_change();
        P_State& p_2 = a2.state_to_change();
        const v3 mom1 = m1 * v1;
        const v3 mom2 = m2 * v2;
        //if (contains(alreadyColliding, mtv)) {
        //std::coutout << "Already colliding " << mtv.id1 << " and " << mtv.id2 << "\n";
        //std::coutout << "From " << id1 << " mom:" << printV(m1*u1) << " and " << id2 << " mom:" << printV(m2*u2) << "\n";
        //std::coutout << "To " << id1 << " mom:" << printV(mom1) << " and " << id2 << " mom:" << printV(mom2) << "\n";
        const float small = 0.0001f;
        const float d1 = glm::length(glm::distance(p1.position + mtv.axis * small, p_2.position));
        const float d2 = glm::length(glm::distance(p1.position - mtv.axis * small, p_2.position));
        if (d2 < d1) {
            mtv.axis = -1.0f * mtv.axis;
            //std::coutout << "Inverting axis\n";
        }

        float overlap = mtv.overlap;
        assert(mtv.overlap >= 0.0f);

        //std::cout << mtv.overlap << "\n";
        if (mtv.overlap <= 0.0001f) continue;

        overlap *= 0.4f;

        if (id1 == 0) {
            std::cout << overlap << "\n";
        }

        v3 f = mtv.axis * overlap;

        const float total_mass = m1+m2;
        const float total_mass_i = 1.0f/total_mass;
        const float m1_ratio = m1 * total_mass_i;
        const float m2_ratio = m2 * total_mass_i;
        float f1_m_mul = total_mass * m2_ratio;
        float f2_m_mul = total_mass * m1_ratio;

        auto velo_changer = [&] (const v3& u, const v3& v) -> float {
            float velo_change = glm::dot(glm::normalize(u),glm::normalize(v));
            if (!std::isnan(velo_change)) {
                //velo_change = std::fabs(velo_change);
                if (velo_change < 0.0f) {
                    velo_change *= 1.4f;
                }
                velo_change = std::fabs(velo_change);
            } else {
                velo_change = 1.0f;
            }
            return velo_change;
        };

        f1_m_mul *= velo_changer(u1,v1);
        f2_m_mul *= velo_changer(u2,v2);

        v3 f1 = f * f1_m_mul;
        v3 f2 = -f * f2_m_mul;

        p_1.set_momentum(mom1);
        p_2.set_momentum(mom2);

        //std::coutout << "ids/forces " << id1 << " " << printV(f1) << ", " << id2 << " " << printV(f2) << "\n";
        forceQueue[id1].push_back(Force(f1,Force::Type::Force,false,true));
        forceQueue[id2].push_back(Force(f2,Force::Type::Force,false,true));

        //std::cout << "Mtv: " << printV(mtv.axis) << " and overlap " << mtv.overlap << "\n";


        // NOTE : can swap m1 and m2 in the forces to cause a light thing flying into a heavy thing
        // to have the light thing fly back proportional to mass of the other
        // with them this way round ie. the force applied to each object is proportional to the other's mass
        // a better sense is achieved of the lighter object coming off worse in a collision
    }

    for (const auto& id_force: forceQueue) {
        const Id& id = id_force.first;
        const std::vector<Force>& forces = id_force.second;
        // all the forces on that actor
        Force force(forces[0]);
        if (forces.size() > 1) {
            for (int i=1; i<forces.size(); ++i) {
                force.force = force.force + forces[i].force; // sum
                // can decide what to do with multiple forces on object here
            }
            //std::coutout << "Dividing force - " << printV(force.force) << " on " << id << " by " << forces.size() << "\n";
            //std::coutout << "Divided force now " << printV(force.force) << "\n";
        }
        actors_.apply_force(id, force);
    }

    for (const auto& mtv: collidingPairs) {
        //std::coutout << "Copying " << a.id1 << "," << a.id2 << " to alreadyColliding\n";
        if (contains(alreadyColliding, mtv)) {
            alreadyColliding[mtv] = ++alreadyColliding[mtv];
        } else {
            alreadyColliding.insert(std::make_pair(mtv,0));
        }
    }

    if (collidingPairs.size() > 0) {
        //std::coutout << "\n";
    } else {
    }
    //long taken = timeNowMicros() - t;
    //std::cout << "Time taken for collision resolving " << (double)(timeNowMicros() - taken)/1000.0 << "ms" << "\n";
    }

    void World::apply_force(const Id& id, const Force& force) {
        actors_.apply_force(id,force);
    }

    void World::render() {
        const Actor& selectedActor = actors_.selectedActor();
        const m4 view = selectedActor.viewMatrix();
        //const G_Cuboid& cam_graphical_cuboid = g_cubs[selectedActor.graphical_cuboid()];
        const int& cam_g_cub = selectedActor.graphical_cuboid();
        assert(contains(g_cubs,cam_g_cub) && "Could not graphical cuboid for viewed/camera");
        const G_Cuboid& cam_graphical_cuboid = g_cubs.find(cam_g_cub)->second;
        const float aspectRatio = windowSize.x / windowSize.y;
        const GLuint viewLoc = glGetUniformLocation(cam_graphical_cuboid.shaderProgram(), "view");

        const m4 projection = glm::perspective(glm::radians(90.0f), aspectRatio, 0.1f, 200.0f);

        long temp = timeNowMicros();

        static std::map<int,std::deque<Id>> graphics_id;

        for (const auto& pai: actors_.underlying()) {
            const Id& id = pai.first;
            const int g_id = pai.second->graphical_cuboid();
            if (!contains(graphics_id,g_id)) {
                graphics_id.emplace(g_id,std::deque<Id>());
            }
            graphics_id[g_id].push_back(id);
        }

        for (const auto& pai: graphics_id) {
            const int& g_id = pai.first;
            const std::deque<Id>& ids = pai.second;
            if (!ids.empty()) {
                const G_Cuboid& graphical_cuboid = g_cubs.find(g_id)->second;

                graphical_cuboid.bindBuffers();
                graphical_cuboid.useShader();

                GLuint projectionLoc = glGetUniformLocation(graphical_cuboid.shaderProgram(), "projection");

                glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
                glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

                glBindVertexArray(graphical_cuboid.VAO);


                // bind buffers
                for (const auto& id: ids) {
                    const Actor& actor = actors_[id];

                    if (!actor.invis()) {
                        // render this shape
                        m4 model = actor.modelMatrix();
                        GLuint modelLoc = glGetUniformLocation(graphical_cuboid.shaderProgram(), "model");
                        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                        glDrawArrays(GL_TRIANGLES, 0, graphical_cuboid.drawSize());

                    }
                }
                glBindVertexArray(0);
                glUseProgram(0);
            }
        }
        // Render end -- -- --

        // honestly if can be bothered an easy optim is just to build this once
        // and only affect changes to it
        graphics_id.clear();

        /*
           for (const auto& pai: g_cubs) {
           const int& g_cub_index = pai.first;
           const G_Cuboid& graphical_cuboid = g_cubs.second;
           for (const auto& a: actors_.underlying()) {
           const G_Cuboid& graphical_cuboid = g_cubs.find((*a.second).graphical_cuboid())->second;
           const Actor& actor = (*a.second);

        //if (true) {
        if (!actor.invis()) {

        graphical_cuboid.bindBuffers();
        graphical_cuboid.useShader();

        GLuint projectionLoc = glGetUniformLocation(graphical_cuboid.shaderProgram(), "projection");

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(graphical_cuboid.VAO);

        m4 model = actor.modelMatrix();
        GLuint modelLoc = glGetUniformLocation(graphical_cuboid.shaderProgram(), "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, graphical_cuboid.drawSize());

        glBindVertexArray(0);
        glUseProgram(0);
        // Render end -- -- --
        }
        }
        }
        */

        //std::cout << "Time for render " << (double)(timeNowMicros()-temp)/1000.0 << "ms\n";

        // 10 minute crosshair
        // An array of 3 vectors which represents 3 vertices
        v2 normed = glm::normalize(windowSize);
        float rat = normed.y/normed.x;
        static GLfloat g_vertex_buffer_data[] = {
            -0.5f*rat, 0.0f, 0.0f,
            0.5f*rat, 0.0f, 0.0f,
            0.0f, 0.5f, 0.0f,
            0.0f, -0.5f, 0.0f
        };

        bool once = false;
        // This will identify our vertex buffer
        static GLuint vertexbuffer;
        if (!once) {
            // Generate 1 buffer, put the resulting identifier in vertexbuffer
            glGenBuffers(1, &vertexbuffer);
            once = true;
        }
        // The following commands will talk about our 'vertexbuffer' buffer
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        // Give our vertices to OpenGL.
        glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

        // 1st attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(
                0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)0            // array buffer offset
                );
        // Draw the triangle !
        glDrawArrays(GL_LINES, 0, 4); // Starting from vertex 0; 3 vertices total -> 1 triangles
        glDisableVertexAttribArray(0);
    }
