#include <vector>
#include <cmath>
#include <glad/glad.h>


struct Verte {
    float x, y, z;  // Position
    float nx, ny, nz;  // Normal
};
std::vector<Verte> createSphereVertices(float radius, unsigned int rings, unsigned int sectors)
{
    std::vector<Verte> vertices;
    const float PI = 3.14159265358979323846f;

    for (unsigned int r = 0; r <= rings; ++r) {
        for (unsigned int s = 0; s <= sectors; ++s) {
            float y = sin(-PI / 2 + PI * r / rings);
            float x = cos(2 * PI * s / sectors) * sin(PI * r / rings);
            float z = sin(2 * PI * s / sectors) * sin(PI * r / rings);

            // Same coordinates for normals, because we're creating a unit sphere
            vertices.push_back({ radius * x, radius * y, radius * z, x, y, z });
        }
    }

    return vertices;
}
struct Sphere {
    GLuint VAO;
    GLuint VBO;
    std::vector<Verte> vertices;

    Sphere(float radius, unsigned int rings, unsigned int sectors)
        : vertices(createSphereVertices(radius, rings, sectors))
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Verte), &vertices[0], GL_STATIC_DRAW);

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Verte), (void*)0);
        glEnableVertexAttribArray(0);
        // normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Verte), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    void Draw(Shader& shader) {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        glBindVertexArray(0);
    }
};
