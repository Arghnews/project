#ifndef MY_G_CUBOID_HPP
#define MY_G_CUBOID_HPP

#include <string>

#include "Util.hpp"
#include "Shader.hpp"

class G_Cuboid {
    private:
        Shader shader;
        const fv* vertex_data;
    public:
        G_Cuboid(const fv* vertexData, std::string vertPath, std::string fragPath);
        GLuint VAO;
        GLuint VBO;
        void bindBuffers();
        void useShader();
        int drawSize();
        GLint shaderProgram();
        ~G_Cuboid();
};
#endif
