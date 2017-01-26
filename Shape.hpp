#ifndef MY_SHAPE_H
#define MY_SHAPE_H
#include "Util.hpp"
#include <GL/glew.h> 
#include <sstream>
#include <vector>
#include <memory>
#include <iostream>
#include <string>

#include "Cuboid.hpp"

class Shape {
    private:
        std::pair<float, float> static project(const v3& axis, const vv3* verts);
        vv3 static getAxes(vv3 v1,vv3 v2);
        Cuboid cuboid_;
        
    public:
        vv3 static getEdges(const vv3& v);
        bool static colliding(Shape&, Shape&);
        ~Shape();
        Shape(const fv* points, const fv* colours, const fv* purple, const fv* green, int id, v3 topCenter,
                std::set<Id> canCollideWith,
                v3 scale=oneV, v3 translationMultiplier=oneV, v3 rotationMultiplier=oneV);
        Cuboid& cuboid();
    
        const fv* points();
        const fv* colours();
        friend std::ostream& operator<<(std::ostream&, const Shape&);
};

#endif
