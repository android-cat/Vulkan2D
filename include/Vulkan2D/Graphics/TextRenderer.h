#pragma once

#include <glm/glm.hpp>
#include <string>
#include <memory>

namespace V2D {

class VulkanContext;
class Font;
class SpriteBatch;
class Camera2D;

enum class TextAlign {
    Left,
    Center,
    Right
};

class TextRenderer {
public:
    TextRenderer(VulkanContext* context);
    ~TextRenderer();

    // Non-copyable
    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    // Draw text using SpriteBatch
    void DrawText(SpriteBatch* batch, Font* font, const std::string& text,
                  const glm::vec2& position, const glm::vec4& color = glm::vec4(1.0f),
                  float scale = 1.0f, TextAlign align = TextAlign::Left);

    void DrawText(SpriteBatch* batch, Font* font, const std::wstring& text,
                  const glm::vec2& position, const glm::vec4& color = glm::vec4(1.0f),
                  float scale = 1.0f, TextAlign align = TextAlign::Left);

    // Draw text with shadow
    void DrawTextWithShadow(SpriteBatch* batch, Font* font, const std::string& text,
                            const glm::vec2& position, const glm::vec4& color,
                            const glm::vec4& shadowColor, const glm::vec2& shadowOffset,
                            float scale = 1.0f, TextAlign align = TextAlign::Left);

    void DrawTextWithShadow(SpriteBatch* batch, Font* font, const std::wstring& text,
                            const glm::vec2& position, const glm::vec4& color,
                            const glm::vec4& shadowColor, const glm::vec2& shadowOffset,
                            float scale = 1.0f, TextAlign align = TextAlign::Left);

private:
    VulkanContext* m_Context;
};

} // namespace V2D
