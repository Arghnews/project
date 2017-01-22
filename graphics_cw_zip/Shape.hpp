#ifndef MY_SHAPE
#define MY_SHAPE
#include "Util.hpp"
#include <sstream>
#include <vector>
#include <GL/glew.h> 
#include <GL/glut.h> 
#include <memory>
#include "Cuboid.hpp"
#include <iostream>
#include <string>
#include <limits>
#include "State.hpp"

class Shape {
    private:
        //void static concat(vv3& grower, const vv3& added);
        std::pair<float, float> static project(const v3& axis, const vv3* verts);
        vv3 static getAxes(vv3 v1,vv3 v2);
        bool _colliding;
        Cuboid _cuboid;
        const fv* _colours;
        const fv* green;
        const fv* purple;
        bool _selected;
        
    public:
        std::set<Id> canCollideWith;
        bool selected();
        void selected(bool b);
        const int id;
        vv3 static getEdges(const vv3& v);
        bool static colliding(Shape&, Shape&);
        ~Shape();
        GLuint VAO;
        GLuint colourVBO();
        std::vector<GLuint> VBOs;
        Shape(const fv* points, const fv* colours, const fv* purple, const fv* green, int id, v3 topCenter,
                std::set<Id> canCollideWith,
                v3 scale=oneV, v3 translationMultiplier=oneV, v3 rotationMultiplier=oneV);
        bool colliding(bool isColliding);
        bool colliding() const;
        Cuboid& cuboid();
    
        const fv* points();
        const fv* colours();
        int name;
        friend std::ostream& operator<<(std::ostream&, const Shape&);

};

#endif
