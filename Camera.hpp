#ifndef CAMERA_H
#define CAMERA_H

class Camera {
    private:
        fq orient;
        m4 trans;

    public:
        Camera();
        void move(const v3& v); // change translation matrix by v

        void turn(const v3& v); // turn by vec, ie. left

        void rotate(const v2& offset); // rotate by mouse input

        m4 update(); // view matrix
};

#endif
