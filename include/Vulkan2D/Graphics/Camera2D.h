#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace V2D {

class Camera2D {
public:
    Camera2D(float width, float height);

    void SetPosition(const glm::vec2& position);
    void SetRotation(float rotation);
    void SetZoom(float zoom);
    void SetViewport(float width, float height);

    const glm::vec2& GetPosition() const { return m_Position; }
    float GetRotation() const { return m_Rotation; }
    float GetZoom() const { return m_Zoom; }

    const glm::mat4& GetViewMatrix() const;
    const glm::mat4& GetProjectionMatrix() const;
    glm::mat4 GetViewProjectionMatrix() const;

    // Camera movement
    void Move(const glm::vec2& delta);
    void Rotate(float delta);
    void Zoom(float delta);

    // Convert screen coordinates to world coordinates
    glm::vec2 ScreenToWorld(const glm::vec2& screenPos) const;
    glm::vec2 WorldToScreen(const glm::vec2& worldPos) const;

private:
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

    glm::vec2 m_Position{0.0f, 0.0f};
    float m_Rotation{0.0f};
    float m_Zoom{1.0f};
    float m_ViewportWidth;
    float m_ViewportHeight;

    mutable glm::mat4 m_ViewMatrix{1.0f};
    mutable glm::mat4 m_ProjectionMatrix{1.0f};
    mutable bool m_ViewDirty{true};
    mutable bool m_ProjectionDirty{true};
};

} // namespace V2D
