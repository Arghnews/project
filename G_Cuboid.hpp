#ifndef MY_G_CUBOID_HPP
#define MY_G_CUBOID_HPP

#include <string>

#include "Util.hpp"
#include "Shader.hpp"

class G_Cuboid {
    protected:
        Shader shader;
        const fv* vertex_data;
    public:
        G_Cuboid(const fv* vertexData, std::string vertPath, std::string fragPath);
        GLuint VAO;
        GLuint VBO;
        virtual void bindBuffers() const;
        void useShader() const;
        int drawSize() const;
        GLint shaderProgram() const;
        virtual ~G_Cuboid();
};

class G_Cuboid_Color : public G_Cuboid {
    protected:
        const fv* color_data;
        GLuint VBO_color;
    public: 
        G_Cuboid_Color(const fv* vertexData, std::string vertPath, std::string fragPath,
                const fv* colorData);
        virtual void bindBuffers() const;
        virtual ~G_Cuboid_Color();
};
#endif
