#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace V2D {

struct Transform2D {
    glm::vec2 position{0.0f, 0.0f};
    glm::vec2 scale{1.0f, 1.0f};
    float rotation{0.0f}; // in radians
    glm::vec2 origin{0.0f, 0.0f}; // pivot point

    glm::mat4 GetMatrix() const {
        glm::mat4 matrix = glm::mat4(1.0f);
        
        // Translate to position
        matrix = glm::translate(matrix, glm::vec3(position, 0.0f));
        
        // Rotate around origin
        matrix = glm::translate(matrix, glm::vec3(origin, 0.0f));
        matrix = glm::rotate(matrix, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        matrix = glm::translate(matrix, glm::vec3(-origin, 0.0f));
        
        // Scale
        matrix = glm::scale(matrix, glm::vec3(scale, 1.0f));
        
        return matrix;
    }

    void Translate(const glm::vec2& delta) {
        position += delta;
    }

    void Rotate(float angle) {
        rotation += angle;
    }

    void SetScale(float uniformScale) {
        scale = glm::vec2(uniformScale);
    }
};

} // namespace V2D
