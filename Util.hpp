#ifndef MY_UTIL
#define MY_UTIL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iostream>
#include <map>
#include <thread>
#include <limits>
#include <math.h>
#include <cmath>
#include <set>
#include <deque>

#define ASIO_STANDALONE
#include "asio.hpp"


// networking stuff
typedef uint8_t Instance_Id;
typedef uint16_t Id;
typedef uint32_t Tick;

typedef uint8_t Seq;
typedef std::deque<Seq> Seqs;

struct Packet_Payload;
typedef std::deque<Packet_Payload> Packet_Payloads;

struct Packet;
typedef std::deque<Packet> Packets;

class Connection;
typedef std::vector<Connection> Connections;

struct Connection_Address;
typedef std::vector<Connection_Address> Connection_Addresses;

typedef glm::fvec2 v2;
typedef glm::fvec3 v3;
typedef glm::fvec4 v4;
typedef glm::fquat fq;
typedef glm::fmat4 m4;
typedef std::vector<GLfloat> fv;
typedef std::vector<v3> vv3;

//typedef std::pair<Id, v3> Id_v3;
//typedef std::pair<Id, fq> Id_fq;

typedef std::pair<v3, Id> v3Id;
typedef std::vector<v3Id> vv3Id;

typedef std::vector<Id> vId;

struct Force;
struct Shot;
typedef std::vector<Force> Forces;
typedef std::vector<Shot> Shots;

typedef asio::io_service io_service;
typedef asio::ip::udp::socket udp_socket;
typedef asio::ip::udp::endpoint udp_endpoint;
typedef std::vector<udp_endpoint> udp_endpoints;

static const v3 FORWARD(0.0f,0.0f,-1.0f);
static const v3 BACKWARD(0.0f,0.0f,1.0f);
static const v3 LEFT(-1.0f,0.0f,0.0f);
static const v3 RIGHT(1.0f,0.0f,0.0f);
static const v3 UP(0.0f,1.0f,0.0f);
static const v3 DOWN(0.0f,-1.0f,0.0f);
//typedef std::pair<float,float> Projection;

static const v3 zeroV(0.0f,0.0f,0.0f);
static const v3 oneV(1.0f,1.0f,1.0f);
static const v3 UP_VECTOR(0.0f,1.0f,0.0f);
static const double PI(M_PI);
static const double HALF_PI(M_PI/2.0f);
static const v3 PI_V3(M_PI,M_PI,M_PI);

static const double EPSILON = 1.0f / 1e6f;

// note: this implementation does not disable this overload for array types
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

bool static hasNan(const v3& v) {
    return std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z);
}

template <typename T>
int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

v3 static vabs(const v3& v) {
    return v3(fabs(v.x),fabs(v.y),fabs(v.z));
}

bool static isZero(const float& v) {
    static const float TINY = 1.0f / 1e12f;
    //static const v3 TINY_V(TINY);
    return fabs(v) < TINY;
}

bool static isZero(const v3& v) {
    static const float TINY = EPSILON * 0.01f;
    //static const v3 TINY_V(TINY);
    return fabs(v.x) < TINY && fabs(v.y) < TINY && fabs(v.z) < TINY;
}

std::string static pr(Seqs& q) {
    std::stringstream buf;
    buf << "(" << q.size() << ") [";
    for (const auto& i: q) {
        buf << int(i) << ", ";
    }
    buf << "]";
    return buf.str();
};

/*
template <class T>
void static concat(std::vector<T>& grower, const std::vector<T>& added) {
    grower.insert( grower.end(), added.begin(), added.end() );
}
*/

template <class T_container_1, class T_container_2>
void static concat(T_container_1& grower, const T_container_2& added) {
    grower.insert( grower.end(), added.begin(), added.end() );
}

// for vector, map
template <class T_container, class T>
bool static contains(const T_container& s, const T& item) {
    return s.find(item) != s.end();
}

// for deque
template <class T>
bool static contains(const std::deque<T>& s, const T& item) {
    //return s.find(item) != s.end();
    return std::find(s.begin(), s.end(), item) != s.end();
}

template <class T_container, class T>
void static erase(T_container& v, const T& ele) {
    v.erase( std::remove(v.begin(), v.end(), ele), v.end() );
}

/*
template <class T>
bool static contains(const std::set<T>& s, const T& item) {
    return s.find(item) != s.end();
}

template <typename K, typename V>
bool static contains(const std::map<K,V>& s, const K& item) {
    return s.find(item) != s.end();
}*/

template <class T>
std::pair<bool,int> static vecContains(const std::vector<std::vector<T>>& vec, const T& t) {
    const int size = vec.size();
    for (int i=0; i<size; ++i) {
        for (int j=0; j<vec[i].size(); ++j) {
            if (vec[i][j] == t) {
                return std::make_pair(true,i);
            }
        }
    }
    return std::make_pair(false,0);
}

std::string static printV(const v3 v) {
    std::stringstream buffer;
    buffer << "(" << v.x << "," << v.y << "," << v.z << ")";
    return buffer.str();
}

std::string static printQ(const fq v) {
    std::stringstream buffer;
    buffer << "( " << v.x << "," << v.y << "," << v.z << ", " << v.w << ")";
    return buffer.str();
}

template <typename N>
bool static areSame(N a, N b, const double epsilon=EPSILON) {
    return fabs(a - b) < epsilon;
}

bool static inline areSame(const v3& a,const v3& b) {
    return areSame(a.x,b.x)&&areSame(a.y,b.y)&&areSame(a.z,b.z);
}

vv3 static unique(const vv3& vec_in, const bool& ignoreSign) {
    vv3 uniq;
    uniq.reserve(8);
    // quick and easy unique directions
    for (unsigned int i=0; i<vec_in.size(); ++i) {
        const auto& elem = vec_in[i];
        bool has = false;

        for (auto& u: uniq) {
            if (!ignoreSign) {
                if (areSame(u,elem)) {
                    has = true;
                    break;
                }
            } else {
                // want to ignore sign
                if (areSame(u,elem) || areSame(u,elem*-1.0f)) {
                    has = true;
                    break;
                }
            }
        }
        if (!has) {
            uniq.emplace_back(elem);
        }
    }
    return uniq;
}

vv3 static unique(const vv3& vec_in) {
    return unique(vec_in, false);
}

long static timeNowSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

long static timeNowMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

long static timeNowMicros() {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

void static sleep_ms(long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(std::max(0l,ms)));
}

void static sleep_us(long us) {
    std::this_thread::sleep_for(std::chrono::microseconds(std::max(0l,us)));
}

#endif
