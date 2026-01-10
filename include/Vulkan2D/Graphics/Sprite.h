#pragma once

#include "Vulkan2D/Math/Transform2D.h"
#include "Vulkan2D/Renderer/Texture.h"
#include "Vulkan2D/Graphics/Color.h"
#include <glm/glm.hpp>
#include <memory>

namespace V2D {

struct SpriteRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 1.0f;
    float height = 1.0f;
};

class Sprite {
public:
    Sprite();
    Sprite(std::shared_ptr<Texture> texture);
    Sprite(std::shared_ptr<Texture> texture, const SpriteRect& sourceRect);

    void SetTexture(std::shared_ptr<Texture> texture);
    void SetSourceRect(const SpriteRect& rect);
    
    // ========================================
    // 色の設定
    // ========================================
    
    /**
     * @brief 色を設定（glm::vec4）
     * @param color RGBA色（0.0-1.0）
     */
    void SetColor(const glm::vec4& color);
    
    /**
     * @brief 色を設定（Colorクラス）
     * @param color Colorオブジェクト
     */
    void SetColor(const Color& color);
    
    /**
     * @brief RGB値で色を設定（0-255）
     * @param r 赤成分（0-255）
     * @param g 緑成分（0-255）
     * @param b 青成分（0-255）
     */
    void SetColorRGB(uint8_t r, uint8_t g, uint8_t b);
    
    /**
     * @brief RGBA値で色を設定（0-255）
     * @param r 赤成分（0-255）
     * @param g 緑成分（0-255）
     * @param b 青成分（0-255）
     * @param a アルファ成分（0-255）
     */
    void SetColorRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    
    /**
     * @brief HTMLカラーコードで色を設定
     * @param hex カラーコード（"#RRGGBB" または "#RRGGBBAA"）
     */
    void SetColorHex(const std::string& hex);
    
    /**
     * @brief アルファ値のみを設定（0.0-1.0）
     * @param alpha アルファ値
     */
    void SetAlpha(float alpha);
    
    void SetSize(const glm::vec2& size);

    std::shared_ptr<Texture> GetTexture() const { return m_Texture; }
    const SpriteRect& GetSourceRect() const { return m_SourceRect; }
    const glm::vec4& GetColor() const { return m_Color; }
    const glm::vec2& GetSize() const { return m_Size; }
    
    Transform2D transform;

    // UV coordinates for the sprite
    glm::vec2 GetUVMin() const;
    glm::vec2 GetUVMax() const;

private:
    std::shared_ptr<Texture> m_Texture;
    SpriteRect m_SourceRect;
    glm::vec4 m_Color{1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec2 m_Size{100.0f, 100.0f};
};

} // namespace V2D
