#ifndef CAMERA_H
#define CAMERA_H

class Camera {
    private:
        dq orient;
        m4 trans;

    public:
        Camera();
        void move(const v3& v); // change translation matrix by v

        void turn(const v3& v); // turn by vec
        //orient = dq(glm::vec3(0.0, -glm::radians(1.0), 0.0)) * orient;

        void rotate(const v2& offset); // rotate by mouse input

        m4 update(); // view matrix
};

#endif
