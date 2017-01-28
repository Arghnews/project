#ifndef CAMERA_H
#define CAMERA_H

class Camera {
    private:
        fq orient;
        m4 trans;
        float speed;
        float slow_speed;
        float fast_speed;

    public:
        Camera();
        Camera(const float& sp);

        v3 pos();

        void toggleSpeed();

        void move(const v3& v); // change translation matrix by v

        void turn(const v3& v); // turn by vec, ie. left

        void rotate(const v2& offset); // rotate by mouse input

        m4 viewMatrix(); // view matrix
        m4 viewMatrx(const v3& pos); // view mat, with camera at pos in world
};

#endif
