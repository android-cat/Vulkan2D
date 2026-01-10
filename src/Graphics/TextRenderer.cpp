/**
 * @file TextRenderer.cpp
 * @brief テキスト描画クラスの実装
 * 
 * フォントアトラスを使用してテキストを描画する
 * 左揃え、中央揃え、右揃え、影付きテキストに対応
 */

#include "Vulkan2D/Graphics/TextRenderer.h"
#include "Vulkan2D/Graphics/Font.h"
#include "Vulkan2D/Renderer/SpriteBatch.h"
#include "Vulkan2D/Renderer/Texture.h"

namespace V2D {

/**
 * @brief テキストレンダラーのコンストラクタ
 * @param context Vulkanコンテキスト
 */
TextRenderer::TextRenderer(VulkanContext* context)
    : m_Context(context) {
}

/**
 * @brief デストラクタ
 */
TextRenderer::~TextRenderer() {
}

/**
 * @brief テキストを描画（string版）
 * @param batch SpriteBatch
 * @param font 使用するフォント
 * @param text 描画する文字列
 * @param position 描画位置（ワールド座標）
 * @param color 文字色
 * @param scale 拡大率
 * @param align 揃え位置（左/中央/右）
 */
void TextRenderer::DrawText(SpriteBatch* batch, Font* font, const std::string& text,
                            const glm::vec2& position, const glm::vec4& color,
                            float scale, TextAlign align) {
    if (!font || text.empty()) return;

    // アライメントに応じたオフセットを計算
    glm::vec2 textSize = font->MeasureText(text) * scale;
    float offsetX = 0.0f;
    
    switch (align) {
        case TextAlign::Center:
            offsetX = -textSize.x / 2.0f;  // 中央揃え
            break;
        case TextAlign::Right:
            offsetX = -textSize.x;  // 右揃え
            break;
        default:
            break;  // 左揃えはオフセット0
    }

    Texture* atlas = font->GetAtlasTexture();
    float x = position.x + offsetX;
    float y = position.y;

    // 各文字を描画
    for (char c : text) {
        const GlyphInfo* glyph = font->GetGlyph(static_cast<uint32_t>(c));
        if (!glyph) {
            // 未知の文字はスペースとして扱う
            x += font->GetFontSize() * scale * 0.5f;
            continue;
        }

        if (glyph->size.x > 0 && glyph->size.y > 0) {
            // グリフの位置とサイズを計算
            float xpos = x + glyph->bearing.x * scale;
            float ypos = y - glyph->bearing.y * scale;
            float w = glyph->size.x * scale;
            float h = glyph->size.y * scale;

            // SpriteBatchを使って描画
            batch->Draw(atlas, 
                       glm::vec2(xpos, ypos), 
                       glm::vec2(w, h),
                       glyph->uvMin, glyph->uvMax,
                       color);
        }

        // 次の文字位置に進む
        x += glyph->advance * scale;
    }
}

/**
 * @brief テキストを描画（wstring版、日本語対応）
 * @param batch SpriteBatch
 * @param font 使用するフォント
 * @param text 描画する文字列（ワイド文字）
 * @param position 描画位置（ワールド座標）
 * @param color 文字色
 * @param scale 拡大率
 * @param align 揃え位置（左/中央/右）
 */
void TextRenderer::DrawText(SpriteBatch* batch, Font* font, const std::wstring& text,
                            const glm::vec2& position, const glm::vec4& color,
                            float scale, TextAlign align) {
    if (!font || text.empty()) return;

    // アライメントに応じたオフセットを計算
    glm::vec2 textSize = font->MeasureText(text) * scale;
    float offsetX = 0.0f;
    
    switch (align) {
        case TextAlign::Center:
            offsetX = -textSize.x / 2.0f;
            break;
        case TextAlign::Right:
            offsetX = -textSize.x;
            break;
        default:
            break;
    }

    Texture* atlas = font->GetAtlasTexture();
    float x = position.x + offsetX;
    float y = position.y;

    // 各文字を描画
    for (wchar_t c : text) {
        const GlyphInfo* glyph = font->GetGlyph(static_cast<uint32_t>(c));
        if (!glyph) {
            x += font->GetFontSize() * scale * 0.5f;
            continue;
        }

        if (glyph->size.x > 0 && glyph->size.y > 0) {
            float xpos = x + glyph->bearing.x * scale;
            float ypos = y - glyph->bearing.y * scale;
            float w = glyph->size.x * scale;
            float h = glyph->size.y * scale;

            batch->Draw(atlas,
                       glm::vec2(xpos, ypos),
                       glm::vec2(w, h),
                       glyph->uvMin, glyph->uvMax,
                       color);
        }

        x += glyph->advance * scale;
    }
}

/**
 * @brief 影付きテキストを描画（string版）
 * @param batch SpriteBatch
 * @param font 使用するフォント
 * @param text 描画する文字列
 * @param position 描画位置
 * @param color 文字色
 * @param shadowColor 影の色
 * @param shadowOffset 影のオフセット
 * @param scale 拡大率
 * @param align 揃え位置
 */
void TextRenderer::DrawTextWithShadow(SpriteBatch* batch, Font* font, const std::string& text,
                                       const glm::vec2& position, const glm::vec4& color,
                                       const glm::vec4& shadowColor, const glm::vec2& shadowOffset,
                                       float scale, TextAlign align) {
    // 最初に影を描画
    DrawText(batch, font, text, position + shadowOffset, shadowColor, scale, align);
    // その上にテキストを描画
    DrawText(batch, font, text, position, color, scale, align);
}

/**
 * @brief 影付きテキストを描画（wstring版）
 */
void TextRenderer::DrawTextWithShadow(SpriteBatch* batch, Font* font, const std::wstring& text,
                                       const glm::vec2& position, const glm::vec4& color,
                                       const glm::vec4& shadowColor, const glm::vec2& shadowOffset,
                                       float scale, TextAlign align) {
    // 最初に影を描画
    DrawText(batch, font, text, position + shadowOffset, shadowColor, scale, align);
    // その上にテキストを描画
    DrawText(batch, font, text, position, color, scale, align);
}

} // namespace V2D
