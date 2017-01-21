#ifndef MY_UTIL
#define MY_UTIL
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
#include <limits>
#include <math.h>

typedef glm::dvec2 v2;
typedef glm::dvec3 v3;
typedef glm::dvec4 v4;
typedef glm::dquat dq;
typedef glm::dmat4 m4;

typedef std::vector<v3> vv3;
//typedef std::pair<float,float> Projection;

static const v3 zeroV(0.0f,0.0f,0.0f);
static const v3 oneV(1.0f,1.0f,1.0f);
static const v3 UP_VECTOR(0.0f,1.0f,0.0f);
static const double PI(M_PI);
static const double HALF_PI(M_PI/2.0f);
static const v3 V3_PI(M_PI,M_PI,M_PI);

static const double EPSILON = 0.001f;

v3 static vabs(const v3& v) {
    return v3(fabs(v.x),fabs(v.y),fabs(v.z));
}

template <class T>
void static concat(std::vector<T>& grower, const std::vector<T>& added) {
    grower.insert( grower.end(), added.begin(), added.end() );
}

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

std::string static printQ(const dq v) {
    std::stringstream buffer;
    buffer << "( " << v.x << "," << v.y << "," << v.z << ", " << v.w << ")";
    return buffer.str();
}

bool static areSame(double a, double b) {
    return fabs(a - b) < EPSILON;
}

bool static areSame(const v3& a,const v3& b) {
    return areSame(a.x,b.x)&&areSame(a.y,b.y)&&areSame(a.z,b.z);
}

vv3 static unique(const vv3 vec_in, const bool ignoreSign) {
    vv3 uniq;
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
                if (areSame(u,elem) || areSame(u,elem*-1.0)) {
                    has = true;
                    break;
                }
            }
        }
        if (!has) {
            uniq.push_back(elem);
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
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

#endif
