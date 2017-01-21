#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>

class Shader {
    public:
        GLuint Program;
        Shader(const GLchar* vertexPath, const GLchar* fragmentPath);
        void Use();
};

#endif
