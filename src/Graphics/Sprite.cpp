/**
 * @file Sprite.cpp
 * @brief スプライト（2D画像）クラスの実装
 * 
 * テクスチャを持つ2Dオブジェクトを表現する
 * 位置、回転、スケール、色などの属性を持つ
 */

#include "Vulkan2D/Graphics/Sprite.h"

namespace V2D {

/**
 * @brief デフォルトコンストラクタ
 */
Sprite::Sprite() {}

/**
 * @brief テクスチャ指定コンストラクタ
 * @param texture 使用するテクスチャ
 * 
 * テクスチャのサイズをスプライトサイズとして使用
 */
Sprite::Sprite(std::shared_ptr<Texture> texture) 
    : m_Texture(texture) {
    if (texture) {
        // テクスチャサイズをスプライトサイズに設定
        m_Size = glm::vec2(static_cast<float>(texture->GetWidth()), 
                          static_cast<float>(texture->GetHeight()));
        // ソース矩形をテクスチャ全体に設定
        m_SourceRect.width = static_cast<float>(texture->GetWidth());
        m_SourceRect.height = static_cast<float>(texture->GetHeight());
    }
}

/**
 * @brief テクスチャとソース矩形指定コンストラクタ
 * @param texture 使用するテクスチャ
 * @param sourceRect テクスチャ内の使用範囲（スプライトシート用）
 */
Sprite::Sprite(std::shared_ptr<Texture> texture, const SpriteRect& sourceRect)
    : m_Texture(texture), m_SourceRect(sourceRect) {
    m_Size = glm::vec2(sourceRect.width, sourceRect.height);
}

/**
 * @brief テクスチャを設定
 * @param texture 新しいテクスチャ
 */
void Sprite::SetTexture(std::shared_ptr<Texture> texture) {
    m_Texture = texture;
    if (texture) {
        m_SourceRect.width = static_cast<float>(texture->GetWidth());
        m_SourceRect.height = static_cast<float>(texture->GetHeight());
    }
}

/**
 * @brief ソース矩形を設定（スプライトシート用）
 * @param rect テクスチャ内の使用範囲
 */
void Sprite::SetSourceRect(const SpriteRect& rect) {
    m_SourceRect = rect;
}

/**
 * @brief 色（ティント）を設定
 * @param color RGBA色（1.0, 1.0, 1.0, 1.0が白/不透明）
 */
void Sprite::SetColor(const glm::vec4& color) {
    m_Color = color;
}

/**
 * @brief 色を設定（Colorクラス）
 * @param color Colorオブジェクト
 */
void Sprite::SetColor(const Color& color) {
    m_Color = color.ToVec4();
}

/**
 * @brief RGB値で色を設定（0-255）
 * @param r 赤成分（0-255）
 * @param g 緑成分（0-255）
 * @param b 青成分（0-255）
 */
void Sprite::SetColorRGB(uint8_t r, uint8_t g, uint8_t b) {
    m_Color = Color::RGB(r, g, b).ToVec4();
}

/**
 * @brief RGBA値で色を設定（0-255）
 * @param r 赤成分（0-255）
 * @param g 緑成分（0-255）
 * @param b 青成分（0-255）
 * @param a アルファ成分（0-255）
 */
void Sprite::SetColorRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    m_Color = Color::RGBA(r, g, b, a).ToVec4();
}

/**
 * @brief HTMLカラーコードで色を設定
 * @param hex カラーコード（"#RRGGBB" または "#RRGGBBAA"）
 */
void Sprite::SetColorHex(const std::string& hex) {
    m_Color = Color::Hex(hex).ToVec4();
}

/**
 * @brief アルファ値のみを設定（0.0-1.0）
 * @param alpha アルファ値
 */
void Sprite::SetAlpha(float alpha) {
    m_Color.a = alpha;
}

/**
 * @brief 描画サイズを設定
 * @param size 幅と高さ（ピクセル）
 */
void Sprite::SetSize(const glm::vec2& size) {
    m_Size = size;
}

/**
 * @brief UV座標の最小値を取得
 * @return テクスチャ座標の左上（0.0〜1.0）
 */
glm::vec2 Sprite::GetUVMin() const {
    if (!m_Texture) return glm::vec2(0.0f);
    
    return glm::vec2(
        m_SourceRect.x / static_cast<float>(m_Texture->GetWidth()),
        m_SourceRect.y / static_cast<float>(m_Texture->GetHeight())
    );
}

/**
 * @brief UV座標の最大値を取得
 * @return テクスチャ座標の右下（0.0〜1.0）
 */
glm::vec2 Sprite::GetUVMax() const {
    if (!m_Texture) return glm::vec2(1.0f);
    
    return glm::vec2(
        (m_SourceRect.x + m_SourceRect.width) / static_cast<float>(m_Texture->GetWidth()),
        (m_SourceRect.y + m_SourceRect.height) / static_cast<float>(m_Texture->GetHeight())
    );
}

} // namespace V2D
