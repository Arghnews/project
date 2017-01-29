#include <GL/glew.h>
#include <string>

#include "Util.hpp"
#include "G_Cuboid.hpp"
#include "Shader.hpp"

/* Graphical data of cuboid
 */

G_Cuboid::G_Cuboid(
    const fv* vertexData, std::string vertPath, std::string fragPath) :
    vertex_data(vertexData), shader(vertPath.c_str(),fragPath.c_str()) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
}

// for now since I don't care about colour I bind the
// positional data as the colour data
void G_Cuboid::bindBuffers() const {

    const fv& vertices = *vertex_data;

    // Bind the Vertex Array Object first, then bind and set vertex bufer(s) and attribute pointer(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // Colour attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); // Unbind VAO
}

GLint G_Cuboid::shaderProgram() const {
    return shader.Program;
}

void G_Cuboid::useShader() const {
    shader.use();
}

int G_Cuboid::drawSize() const {
    return vertex_data->size();
}

G_Cuboid::~G_Cuboid() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}
