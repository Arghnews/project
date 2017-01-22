#include <GL/glew.h> 
#include <GL/glut.h> 
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <math.h>
#include <thread>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <set>
#include <chrono>
#include <thread>
#include <algorithm>
#include <set>
#include <deque>
#include <stdexcept>

#include "crap.hpp"
#include "Util.hpp"
#include "Shape.hpp"
#include "Octtree.hpp"
#include "AABB.hpp"
#include "Movement.hpp"
#include "State.hpp"

struct Lerp {
    Id shape;
    fq from;
    fq to;
};

void idle();
int init(int argc, char* argv[]);
void createShapes();
void render();
void bindBuffers(Shapes& shapes);
void bindBuffers(GLuint VAO, std::vector<GLuint> VBOs, const fv* vertexData, const fv* colourData);
void mouseClicks(int button, int state, int x, int y);
void startLoopGl();
void collisions();
void keyboard(unsigned char key, int mouseX, int mouseY);
void keyboard_up(unsigned char key, int x, int y);
void specialInput(int key, int x, int y);
void specialInputUp(int key, int x, int y);
void cleanupAndExit();
void switchShapes(int);
std::vector<Shape*> getShapes();
Movements processMovements();
State rotateShape(Id shape, const v3& rotateBy);
State rotateShape(Id shape, const fq& quat);
State setOrient(Id shape, const fq& quat);
State translateShape(Id shape, const v3& translate);
void extraShapes();
void updateKey(char c, bool state);
void processKeystates();
void moveCamera(v3 camera_movement_multiplier);
std::vector<Lerp> calcClawLerps();

static const float fps = 60.0f;
static v3 camera_velocity(0.15f,0.15f,0.15f);
// make sure these aren't same otherwise nan's
static v3 camera_lookingAt(-1.5f,-8.0f,-3.0f); // eye, coordinate in world
static v3 camera_position(1.5f,9.0f,3.0f);  // center, where looking at
static float mouseX;
static float mouseY;
static std::map<char, bool> keys;

static bool allowCollision = false;

static int selectedPartIndex = 0;
//static const int numbShapes = 2;
static const Id base = 0;
static const Id base_shoulder_connector = 1;
static const Id shoulder = 2;
static const Id shoulder_arm_connector = 3;
static const Id arm = 4;
static const Id arm_forearm_connector = 5;
static const Id forearm = 6;
static const Id forearm_platter_connector = 7;
static const Id platter = 8;
static const Id claw1 = 9;
static const Id claw2 = 10;
static std::vector<v3> claw_offsets;

static const std::vector<Id> CLAW_PARTS = {
    claw1, claw2
};

std::vector<std::vector<Id>> ARM_PARTS = {
    {base},
    {base_shoulder_connector},
    {shoulder},
    {shoulder_arm_connector},
    {arm},
    {arm_forearm_connector},
    {forearm},
    {forearm_platter_connector},
    {platter},
    CLAW_PARTS,
    {}
};

GLuint shaderProgram;
Shapes shapes;
float step = 0.2f; // for movement

static const float areaSize = 100.0f;
Octtree bigTree(v3(0.0f,0.0f,0.0f),areaSize);

Movements movements;

void createShapes() {

    v3 bottom_upRightTop = v3(0.0f, 1.0f, 0.0f);
    v3 center_upRightTop = v3(0.0f, 0.5f, 0.0f);
    v3 connector_dims(0.1f,0.5f,0.1f);

//Shape::Shape(const fv* points, const fv* colours, const fv* purple, const fv* green,
//int id, v3 topCenter, std::set<Id> canCollideWith, v3 scale, v3 translationMultiplier, v3 rotationMultiplier)
    
    std::set<Id> canCollideWith = {};
    // base
    shapes[base] = (new Shape(&cubePointsCentered,
            &cubeColours,&cubeColoursPurple,&cubeColoursGreen,
            base,center_upRightTop,canCollideWith,
            v3(5.0f,1.0f,5.0f),v3(1.0f,0.0f,1.0f),v3(0.0f,1.0f,0.0f)));

    canCollideWith = {base, shoulder};
    // base->shoulder
    shapes[base_shoulder_connector] = (new Shape(&cubePointsBottom,
            &cubeColours,&cubeColoursPurple,&cubeColoursGreen,
            base_shoulder_connector,bottom_upRightTop,canCollideWith,
            connector_dims,zeroV));

    // shoulder
    canCollideWith = {};
    shapes[shoulder] = (new Shape(&cubePointsBottom,
            &cubeColours,&cubeColoursPurple,&cubeColoursGreen,
            shoulder,bottom_upRightTop,canCollideWith,
            v3(0.3f,4.5f,0.3f),zeroV));

    canCollideWith = {shoulder, arm};
    // shoulder->arm
    shapes[shoulder_arm_connector] = (new Shape(&cubePointsBottom,
            &cubeColours,&cubeColoursPurple,&cubeColoursGreen,
            shoulder_arm_connector,bottom_upRightTop,canCollideWith,
            connector_dims,zeroV));

    // arm
    canCollideWith = {};
    shapes[arm] = (new Shape(&cubePointsBottom,
            &cubeColours,&cubeColoursPurple,&cubeColoursGreen,
            arm,bottom_upRightTop,canCollideWith,
            v3(0.3f,3.5f,0.3f),zeroV));

    // arm->forearm
    canCollideWith = {arm,forearm};
    shapes[arm_forearm_connector] = (new Shape(&cubePointsBottom,
            &cubeColours,&cubeColoursPurple,&cubeColoursGreen,
            arm_forearm_connector,bottom_upRightTop,canCollideWith,
            connector_dims,zeroV));

    // forearm
    canCollideWith = {};
    shapes[forearm] = (new Shape(&cubePointsBottom,
            &cubeColours,&cubeColoursPurple,&cubeColoursGreen,
            forearm,bottom_upRightTop,canCollideWith,
            v3(0.3f,5.5f,0.3f),zeroV));

    // forearm->platter
    canCollideWith = {forearm,platter};
    shapes[forearm_platter_connector] = (new Shape(&cubePointsBottom,
            &cubeColours,&cubeColoursPurple,&cubeColoursGreen,
            forearm_platter_connector,bottom_upRightTop,canCollideWith,
            1.25f*connector_dims,zeroV));

    // platter
    canCollideWith = {};
    shapes[platter] = (new Shape(&cubePointsCentered,
            &cubeColours,&cubeColoursPurple,&cubeColoursGreen,
            platter,center_upRightTop,canCollideWith,
            v3(5.5f,0.25f,5.5f),zeroV));

    const v3 clawDimensions = v3(0.2f,3.0f,0.2f);
    // claw1
    canCollideWith = {};
    shapes[claw1] = (new Shape(&cubePointsBottom,
            &cubeColours,&cubeColoursPurple,&cubeColoursGreen,
            claw1,bottom_upRightTop,canCollideWith,
            clawDimensions,zeroV));

    // claw2
    canCollideWith = {};
    shapes[claw2] = (new Shape(&cubePointsBottom,
            &cubeColours,&cubeColoursPurple,&cubeColoursGreen,
            claw2,bottom_upRightTop,canCollideWith,
            clawDimensions,zeroV));

    v3 gap(0.0f,0.05f,0.0f); // gap between things
    // assumes platter square
    const v3 halfDimensions = shapes[platter]->cuboid().half_xyz();
    const float x_d = halfDimensions.x;
    const float z_d = halfDimensions.z;
    float platter_diag_flat = sqrt((x_d*x_d + z_d*z_d));

    for (auto& s: shapes) {
        auto& shape = s.second;
        bigTree.insert(shape->cuboid().state().pos,shape);
    }

    // claws
    // move claw to side
    auto claw_offset = v3(platter_diag_flat,0.0f,0.0f);
    //auto claw1_offset = zeroV;
    claw_offsets.push_back(claw_offset);
    claw_offsets.push_back(claw_offset * -1.0f);
    translateShape(claw1,claw_offsets[0]+gap);
    translateShape(claw2,claw_offsets[1]+gap);

    v3 height;
    for (auto& s: shapes) {
        Id id = s.first;
        Shape& shape = *s.second;
        translateShape(id, height);
        v3 myHeight = shape.cuboid().state().topCenter;
        if (id == base) {
            height += shape.cuboid().state().topCenter;
        } else if (id == platter) {
            height += shape.cuboid().state().topCenter;
        }
        if (!(std::find(CLAW_PARTS.begin(),CLAW_PARTS.end(), id) != CLAW_PARTS.end())) {
            height += myHeight;
        }
    }

    int otherShape = shapes.size();
    shapes[otherShape] = (new Shape(&cubePointsCentered,
            &cubeColours,&cubeColoursPurple,&cubeColoursGreen,
            otherShape,center_upRightTop,canCollideWith,
            v3(2.0f,1.0f,1.5f),zeroV));
    translateShape(otherShape, v3(7.0f,1.0f,3.0f));
    otherShape += 1;
    shapes[otherShape] = (new Shape(&cubePointsCentered,
            &cubeColours,&cubeColoursPurple,&cubeColoursGreen,
            otherShape,center_upRightTop,canCollideWith,
            v3(1.0f,2.0f,2.5f),zeroV));
    translateShape(otherShape, v3(10.0f,0.2f,5.0f));

    for (auto& s: shapes) {
        auto& shape = s.second;
        glGenVertexArrays(1, &(shape->VAO));
        glGenBuffers(1, &(shape->VBOs[0])); // vertex
        glGenBuffers(1, &(shape->VBOs[1])); // colour
    }

}

State setOrient(Id s, const fq& quat) {
    if (shapes.count(s) == 0) {
        std::string err = "No element with id " + std::to_string(s) + " in map";
        throw std::runtime_error(err);
    }
    auto r = shapes[s]->cuboid().setOrient(quat);
    return r;
}

State rotateShape(Id s, const fq& quat) {
    if (shapes.count(s) == 0) {
        std::string err = "No element with id " + std::to_string(s) + " in map";
        throw std::runtime_error(err);
    }
    auto r = shapes[s]->cuboid().rotateQuat(quat);
    return r;
}

State rotateShape(Id s, const v3& rotateBy) {
    if (shapes.count(s) == 0) {
        std::string err = "No element with id " + std::to_string(s) + " in map";
        throw std::runtime_error(err);
    }
    auto r = shapes[s]->cuboid().rotateRads(rotateBy);
    return r;
}

State translateShape(Id s, const v3& translate) {
    if (shapes.count(s) == 0) {
        std::string err = "No element with id " + std::to_string(s) + " in map";
        throw std::runtime_error(err);
    }
    Shape& shape = *shapes[s];
    const bool deleted = bigTree.del(shape.cuboid().state().pos,&shape);
    auto worked = shape.cuboid().translate(translate);
    bigTree.insert(shape.cuboid().state().pos,&shape);
    return worked;
}

void moveCamera(v3 camera_movement_multiplier) {
    camera_position += camera_movement_multiplier * camera_velocity * glm::normalize(camera_lookingAt);
}

void processKeystates() {
    v3 translate(zeroV);
    v3 rotateV(zeroV);
    v3 camera_movement_multiplier(zeroV);
    bool stop = false;

    const float ang = glm::radians(40.0f)/fps;
    const float trans(3.0f/fps);

    if (keys['r'])
        rotateV = v3(ang,0.0f,0.0f);
    if (keys['R'])
        rotateV = v3(-ang,0.0f,0.0f);
    if (keys['y'])
        rotateV = v3(0.0f,ang,0.0f);
    if (keys['Y'])
        rotateV = v3(0.0f,-ang,0.0f);
    if (keys['z'])
        rotateV = v3(0.0f,0.0f,ang);
    if (keys['Z'])
        rotateV = v3(0.0f,0.0f,-ang);
    if (keys['h'])
        translate = v3(-trans,0.0f,0.0f);
    if (keys['l'])
        translate = v3(trans,0.0f,0.0f);
    if (keys['k'])
        translate = v3(0.0f,0.0f,trans);
    if (keys['j'])
        translate = v3(0.0f,0.0f,-trans);
    if (keys['w'])
        camera_movement_multiplier = oneV;
    if (keys['W'])
        camera_movement_multiplier = oneV;
    if (keys['s'])
        camera_movement_multiplier = -oneV;
    if (keys['S'])
        camera_movement_multiplier = -oneV;
    if (keys[static_cast<char>(27)])
        stop = true; // escape key
    if (keys[GLUT_KEY_LEFT])
        switchShapes(-1);
    if (keys[GLUT_KEY_RIGHT])
        switchShapes(1);
    if (keys[static_cast<char>(9)])
        switchShapes(1); // tab

    static const float interp_const = 0.4f / fps;

    static bool clawMoving = false;
    static std::vector<Lerp> lerps;
    static float interp = 0.0f;
    if (keys[GLUT_LEFT_BUTTON]) {
        if (!clawMoving) {
            lerps = calcClawLerps();
            clawMoving = true;
        } 
        for (const auto& l: lerps) { // move each shape
            const fq rotation = glm::mix(l.from, l.to, interp);
            Movement m(l.shape, Movement::Transform::Orient, rotation);
            movements.push_back(m);
        }
        interp += interp_const;
    } 
    if (!keys[GLUT_LEFT_BUTTON]) {
        clawMoving = false;
        interp = 0.0f;
    }
    
    if (keys[GLUT_RIGHT_BUTTON]) {
        // reset claw
        State platter_state = shapes[platter]->cuboid().state();
        for (auto& id: CLAW_PARTS) {
            Movement reset(id, Movement::Transform::Orient, platter_state.orient);
            movements.push_back(reset);
        }
    }
    
    moveCamera(camera_movement_multiplier);

    for (auto& s: getShapes()) { // for every shape selected
        Shape& shape = *s;
        const Id id = shape.id;
        if (translate != zeroV) {
            // for some shapes translationMultiplier will be 0, so cannot move
            const v3 translateMultiplier = shape.cuboid().translationMultiplier;
            Movement m(id, Movement::Transform::Translation, translate * translateMultiplier);
            movements.push_back(m);
        } else if (rotateV != zeroV) {
            if (!(std::find(CLAW_PARTS.begin(), CLAW_PARTS.end(), id) != CLAW_PARTS.end())) {
                // if not a claw part
                const v3 rotationMultiplier = shape.cuboid().rotationMultiplier;
                Movement m(id, Movement::Transform::Rotation, rotationMultiplier * rotateV);
                movements.push_back(m);
            }
        }
    }

    if (stop) {
        cleanupAndExit();
    }
}

std::vector<Lerp> calcClawLerps() {
    const Id& platterId = platter;
    // if a platter exists - which it bloody should
    const bool platterExists = vecContains(ARM_PARTS, platterId).first && shapes.count(platterId) > 0;
    assert(platterExists && "No platter for claws to move relative to");
    std::vector<Lerp> lerps;
    for (const auto& claw_part_id: CLAW_PARTS) {
        const Id& clawId = claw_part_id;
        const State& platterState = shapes[platterId]->cuboid().state();
        const State& clawState = shapes[clawId]->cuboid().state();
        const v3 v1 = platterState.topCenter + platterState.pos - clawState.pos;
        const v3 v2 = clawState.topCenter;

        const fq q = glm::normalize(glm::rotation(glm::normalize(v2),glm::normalize(v1)));
        const fq q1 = glm::normalize(clawState.orient); // start state
        const fq q2 = glm::normalize(q * q1); // result orient
        Lerp l;
        l.shape = clawId;
        l.from = q1;
        l.to = q2;
        lerps.push_back(l);
    }
    return lerps;
}

// char is key, state->true is down, state->false is up
void updateKey(char c, bool state) {
    keys[c] = state;
}

void main_loop() {

    static long frame = 0l;
    static long totalTimeTaken = 0l;
    static long timeTakenInterval = 0l;
    ++frame;

    long startTime = timeNowMicros();

    startLoopGl();
    processKeystates();
    
    collisions();

    bindBuffers(shapes);
    render();

    float fullFrametime = (1000.0f*1000.0f)/fps;
    long timeTaken = timeNowMicros() - startTime;
    int sleepTime = std::max((int)(fullFrametime - timeTaken),0);
    std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
}

Movements processMovements() {

    Movements movements_done;

    for (int i=0; i<movements.size(); ++i) {

        std::deque<Movement> thisMove;

        Movement& m = movements[i];
        State originalMove = m.state;
        const Id& id = m.shape;
        const State& before = shapes[id]->cuboid().state();
        State difference = m.move();
        const State& after = shapes[id]->cuboid().state();

        const auto has_index = vecContains(ARM_PARTS, id); // O(n^2), but list is always tiny
        const bool has = has_index.first;
        const int where = has_index.second;
        const bool valid_place = !(where==0 && ARM_PARTS.size()==1) && (where < ARM_PARTS.size()-1);
        const bool chain = has && valid_place;

        if (chain) { // chaining movement of linked objects
            const Id parent = where;
            const Id child_start = parent + 1;
            const int chain_size = ARM_PARTS.size();

            if (m.t == Movement::Transform::Rotation) {
                const v3 parent_pos = after.pos;
                const fq q(originalMove.rotation);
                for (int j=child_start; j<chain_size; ++j) {
                    const auto& children = ARM_PARTS[j];
                    for (int k=0; k<children.size(); ++k) {
                        const Id this_child = children[k];
                        const State childState = shapes[this_child]->cuboid().state();
                        const v3 my_pos = childState.pos;
                        const v3 rotated_point = parent_pos + (q * (my_pos - parent_pos) );
                        const v3 translationVec = rotated_point - childState.pos;
                        Movement mTrans(this_child, Movement::Transform::Translation, translationVec);
                        Movement mRotate(this_child, Movement::Transform::Rotation, originalMove);
                        thisMove.push_back(mRotate);
                        thisMove.push_back(mTrans);
                    }
                }
            } else if (m.t == Movement::Transform::Translation) {
                for (int j=child_start; j<chain_size; ++j) {
                    const auto& children = ARM_PARTS[j];
                    for (int k=0; k<children.size(); ++k) {
                        const Id this_child = children[k];
                        Movement mTrans(this_child, Movement::Transform::Translation, originalMove.pos);
                        thisMove.push_back(mTrans);
                    }
                }
            }
        }
        movements_done.push_back(m);

        while (!thisMove.empty()) {
            Movement& m = thisMove.front();
            m.move();
            movements_done.push_back(m);
            thisMove.pop_front();
        }

    }
    return movements_done;
}

void collisions() {

    Movements moves_made = processMovements();

    std::set<Id> collidingSet;
    std::set<Id> notCollidingSet;
    for (const auto& s: shapes) {
        Shape& shape = *s.second;
        notCollidingSet.insert(shape.id);
    }
    std::set<std::pair<Id,Id>> collidingPairs;

    const int size = shapes.size();
    for (auto& s: shapes) {
        Shape& shape = *s.second;
        const v3 pos = shape.cuboid().state().pos;
        //just use one for now, will change so that shapes
        // store their max dimensions
        const float halfDimensions = shape.cuboid().furthestVertex()*2.0f;
        vv3S shapes_nearby = bigTree.queryRange(pos, halfDimensions);

        for (auto& s_n: shapes_nearby) {
            auto& nearby_shape = *s_n.second;

            if (&shape == &nearby_shape) {
                continue;
            }

            const bool collidingNow = Shape::colliding(shape, nearby_shape);
            if (collidingNow) {
                // collision
                collidingSet.insert(shape.id);
                collidingSet.insert(nearby_shape.id);
                notCollidingSet.erase(shape.id);
                notCollidingSet.erase(nearby_shape.id);
                const std::pair<Id,Id> colShapes = std::make_pair(
                        std::min(shape.id,nearby_shape.id),std::max(shape.id,nearby_shape.id));
                collidingPairs.insert(colShapes);
            } else {
                // no collision
            }
        }
    }

    // for now simple undo all approach if any collide ----
    // however later can implement a system where in the movements above, each move made by say the shoulder
    // affects the arm, and both these id's are recorded and put into lists
    // then if the arm affects something else, this also into list etc
    // and then down here can check if anything in the collidingPairs is in a list,
    // to undo those lists of all affected objects

    bool needUndo = false;
    
    // for things that should stick together -
    // if both claws touching a cube that isn't in the arm anywhere
    // map of things touching claws -> number of claws touched
    // only if thing A touches CLAW_PARTS.size() then attach it to vector of parts

    std::map<Id,int> hittingAllClaws;
    for (auto& pai: shapes) {
        // everything hitting 0 times
        hittingAllClaws.insert(std::make_pair(pai.first,0));
    }
    for (auto& pai: collidingPairs) {
        const Id& shape1Id = pai.first;
        const Id& shape2Id = pai.second;
        const Shape& shape1 = *shapes[shape1Id];
        const Shape& shape2 = *shapes[shape2Id];
        // if object hitting a claw part, add it to above set
        const bool is1claw = std::find(CLAW_PARTS.begin(), CLAW_PARTS.end(), shape1Id) != CLAW_PARTS.end();
        const bool is2claw = std::find(CLAW_PARTS.begin(), CLAW_PARTS.end(), shape2Id) != CLAW_PARTS.end();
        if (is1claw ^ is2claw) {
            if (is1claw) {
                hittingAllClaws[shape2Id] += 1;
            } else {
                hittingAllClaws[shape1Id] += 1;
            }
        }
    }

    const int CLAW_PARTS_SIZE = CLAW_PARTS.size();

    std::set<Id> just_grabbed;
    static std::set<Id> grabbed;
    std::set<Id> just_released;

    // map of id -> number of hits on claws
    for (auto& pai: hittingAllClaws) {
        // part is grabbed by all claw parts
        if (pai.second == CLAW_PARTS_SIZE) {
            // if not already grabbed, add to just_grabbed
            if (!(grabbed.find(pai.first) != grabbed.end())) {
                just_grabbed.insert(pai.first);
            }
            // add to full grabbed list
            grabbed.insert(pai.first);
        } else {
            // if it was grabbed and now it's not hitting any claws
            const bool was_grabbed = grabbed.find(pai.first) != grabbed.end();
            if (was_grabbed) {
                just_released.insert(pai.first);
            }
            grabbed.erase(pai.first);
        }
    }

    std::vector<Id>& GRABBED_SHAPES = ARM_PARTS.back();

    for (auto& id: just_grabbed) {
        std::cout << "Just grabbed " << id << "\n";
        GRABBED_SHAPES.push_back(id);
    }
    for (auto& id: just_released) {
        GRABBED_SHAPES.erase(std::remove(GRABBED_SHAPES.begin(), GRABBED_SHAPES.end(), id), GRABBED_SHAPES.end());
        std::cout << "Just released " << id << "\n";
    }

    for (auto& pai: collidingPairs) {
        const Id& shape1Id = pai.first;
        const Id& shape2Id = pai.second;
        const Shape& shape1 = *shapes[shape1Id];
        const Shape& shape2 = *shapes[shape2Id];
        //const bool allowedToCollideGlobal = allowCollision; // global for now
        //static const bool allowedToCollideGlobal = false; // global for now
        const bool grabbed_1 = grabbed.find(shape1Id) != grabbed.end();
        const bool grabbed_2 = grabbed.find(shape2Id) != grabbed.end();
        const bool allowedToCollide = grabbed_1 || grabbed_2;
        std::set<Id> shape1CanHit = shape1.canCollideWith;
        const bool can1Hit2 = shape1CanHit.find(shape2Id) != shape1CanHit.end();
        std::set<Id> shape2CanHit = shape2.canCollideWith;
        const bool can2Hit1 = shape2CanHit.find(shape1Id) != shape2CanHit.end();
        const bool mayCollide = can1Hit2 || can2Hit1;
        if (!mayCollide && !allowedToCollide) {
            needUndo = true;
            break;
        }
    }

    if (needUndo) {
        for (int i = moves_made.size(); i-- > 0;) {
            moves_made[i].undo();
        }
    }
    
    for (auto& id: collidingSet) {
        shapes[id]->colliding(true);
    }
    for (auto& id: notCollidingSet) {
        shapes[id]->colliding(false);
    }

    movements.clear();
}

void render() {
    for (auto& s: shapes) {
        Shape& shape = *s.second;
        auto pos = shape.cuboid().state().pos;
        auto qua = shape.cuboid().state().orient;
        auto sca = shape.cuboid().scale();
        glBindVertexArray(shape.VAO);
        //
        // local space -> world space -> view space -> clip space -> screen space
        //          model matrix   view matrix  projection matrix   viewport transform
        // Vclip = Mprojection * Mview * Mmodel * Vlocal

        float aspectRatio = (float)(glutGet(GLUT_WINDOW_WIDTH) / glutGet(GLUT_WINDOW_HEIGHT));

        m4 model;
        m4 trans;
        m4 rotateM = glm::mat4_cast(qua);
        m4 scale;

        trans = glm::translate(trans, pos);
        scale = glm::scale(scale, sca);  
        model = trans * rotateM * scale;

        glm::mat4 view;
        // Note that we're translating the scene in the reverse direction of where we want to move
        //view = glm::translate(view, v3(0.0f, 0.0f, -3.0f)); 
        view = glm::lookAt(
                camera_position, // eye, coordinate in world
                camera_position + camera_lookingAt,  // center, where looking at
                UP_VECTOR); // up

        glm::mat4 projection;
        projection = glm::perspective(glm::radians(80.0f), aspectRatio, 0.1f, 200.0f);
        //projection = glm::ortho(-3.0f,3.0f,-3.0f,3.0f,0.1f, 100.0f);

        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glDrawArrays(GL_TRIANGLES, 0, shape.points()->size());
    }
    glBindVertexArray(0);
    glutSwapBuffers(); 
}

void mouseClicks(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        updateKey(GLUT_LEFT_BUTTON, true);
    } else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        updateKey(GLUT_LEFT_BUTTON, false);
    } else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        updateKey(GLUT_RIGHT_BUTTON, true);
    } else if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP) {
        updateKey(GLUT_RIGHT_BUTTON, false);
    }
}

void mouseMove(int x, int y) {
    int cX = glutGet(GLUT_WINDOW_WIDTH) / 2;                                     
    int cY = glutGet(GLUT_WINDOW_HEIGHT) / 2;                                    
    if (cX != x || cY != y) {                                                    
        glutWarpPointer(cX, cY);                                                 
    }

    auto mouseOffset = glm::vec2(x, cY) - glm::vec2(cX, y);                      
	float halfWidth = glutGet(GLUT_WINDOW_WIDTH)/2.0f;
	float halfHeight = glutGet(GLUT_WINDOW_HEIGHT)/2.0f;

	float xPos = x/halfWidth - 1.0f;
	float yPos = 1.0f - y/halfHeight;
	float oldCursorX = mouseX;
	float oldCursorY = mouseY;
    float deltaX = mouseOffset.x/halfWidth;
    float deltaY = mouseOffset.y/halfHeight;
	static const float PI_BY_EIGHTEEN = M_PI/18.0f; 
	static const float PI_BY_EIGHTEEN_TIMES_SEVENTEEN = 17.0f * M_PI/18.0f; 
	glm::vec3 lookingAtCopy = camera_lookingAt;

	// don't want to move too far, ie. from 169 degrees up round to bottom, if such a move, won't do it
	float newAngle = std::acos(glm::dot(lookingAtCopy,UP_VECTOR))-deltaY;
	if (newAngle > PI_BY_EIGHTEEN && newAngle < PI_BY_EIGHTEEN_TIMES_SEVENTEEN) {
		lookingAtCopy = glm::normalize(glm::rotate(lookingAtCopy, (float)deltaY, glm::cross(lookingAtCopy,UP_VECTOR))); // y
	}
	lookingAtCopy = glm::normalize(glm::rotate(lookingAtCopy, -(float)(deltaX), UP_VECTOR)); // x
	camera_lookingAt = lookingAtCopy;

	mouseX += deltaX;
	mouseY += deltaY;
}

void bindBuffers(Shapes& shapes) {
    for (auto& s: shapes) {
        auto& shape = s.second;
        bindBuffers(shape->VAO, shape->VBOs, shape->points(), shape->colours());
    }
}

void bindBuffers(GLuint VAO, std::vector<GLuint> VBOs, const fv* vertexData, const fv* colourData) {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, vertexData->size()*sizeof(GLfloat), 
            vertexData->data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 
            3 * sizeof(GLfloat), (GLvoid*)(0*sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
    glBufferData(GL_ARRAY_BUFFER, colourData->size()*sizeof(GLfloat), 
            colourData->data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 
            3 * sizeof(GLfloat), (GLvoid*)(0*sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void keyboard(unsigned char key, int mouseX, int mouseY) {
    updateKey(key, true);
}

void keyboard_up(unsigned char key, int x, int y) {
    updateKey(key, false);
}

void specialInput(int key, int x, int y) {
    updateKey(key, true);
}

void specialInputUp(int key, int x, int y) {
    updateKey(key, false);
}

std::vector<Shape*> getShapes() {
    std::vector<Shape*> selectedShapes;
    // turn off all
    for (auto& ARM_PART: ARM_PARTS) {
        for (auto& piece_Id: ARM_PARTS[selectedPartIndex]) {
            shapes[piece_Id]->selected(false);
        }
    }
    // select appropriate parts and turn on and return
    for (auto& piece_part: ARM_PARTS[selectedPartIndex]) {
        shapes[piece_part]->selected(true);
        selectedShapes.push_back(shapes[piece_part]);
    }
    return selectedShapes;
}

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

void switchShapes(int by) {
    // turn off all
    for (auto& ARM_PART: ARM_PARTS) {
        for (auto& piece_Id: ARM_PARTS[selectedPartIndex]) {
            shapes[piece_Id]->selected(false);
        }
    }
    static long lastTime = timeNowMillis();
    long timeSinceLast = timeNowMillis() - lastTime;
    const long interval = 125l; // quarter of a second, ms
    if (timeSinceLast > interval) {
        // update selected part
        selectedPartIndex += by;
        if (selectedPartIndex % 2 != 0) {
            selectedPartIndex += sgn(by);
        }
        selectedPartIndex %= ARM_PARTS.size();
        lastTime = timeNowMillis();
    }
    // turn on, highlight, said part
    for (auto& piece_part: ARM_PARTS[selectedPartIndex]) {
        shapes[piece_part]->selected(true);
    }
}

int main(int argc, char* argv[]) {
    int success = init(argc, argv);
    if (success != 0) {
        return success;
    }

    shaderProgram = shaders();
    createShapes();

    glUseProgram(shaderProgram);
    glutMainLoop(); 

    // never leaves main loop...

    cleanupAndExit();

    return 0; 
}

void cleanupAndExit() {
    for (auto& s: shapes) {
        auto& shape = s.second;
        glDeleteVertexArrays(1, &shape->VAO);
        glDeleteBuffers(1, &shape->VBOs[0]);
        glDeleteBuffers(1, &shape->VBOs[1]);
    }
    for (auto& s: shapes) {
        auto& shape = s.second;
        delete shape;
    }
    shapes.clear();
    exit(0);
}

void startLoopGl() {
    glEnable(GL_DEPTH_TEST);
    // Uncommenting this call will result in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); 
}

int init(int argc, char* argv[]) {
	glutInit(&argc, argv); 
	glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH); 

	glutInitWindowSize(800, 800); 
	glutInitWindowPosition(50, 50); 

	glutCreateWindow("Robot arm-y"); 
	
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    glutDisplayFunc(main_loop); 
    glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);
	glutKeyboardFunc(keyboard); 
    glutKeyboardUpFunc(keyboard_up);
	glutIdleFunc(idle); 
    glutSpecialFunc(specialInput); 
    glutSpecialUpFunc(specialInputUp); 
    glutMouseFunc(mouseClicks);
    glutMotionFunc(mouseMove);
    glutPassiveMotionFunc(mouseMove);
    glutSetCursor(GLUT_CURSOR_NONE);
    
    return 0;
}

void idle() {
    glutPostRedisplay();
}

