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

// UTF-8文字列からUnicodeコードポイントを抽出するヘルパー関数
static std::vector<uint32_t> Utf8ToCodepoints(const std::string& utf8String) {
    std::vector<uint32_t> codepoints;
    size_t i = 0;
    while (i < utf8String.length()) {
        uint32_t codepoint = 0;
        uint8_t c = static_cast<uint8_t>(utf8String[i]);
        
        if ((c & 0x80) == 0) {
            // 1バイト文字 (0xxxxxxx)
            codepoint = c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            // 2バイト文字 (110xxxxx 10xxxxxx)
            if (i + 1 < utf8String.length()) {
                codepoint = ((c & 0x1F) << 6) |
                           (static_cast<uint8_t>(utf8String[i + 1]) & 0x3F);
                i += 2;
            } else {
                i++; // 不正なシーケンス
            }
        } else if ((c & 0xF0) == 0xE0) {
            // 3バイト文字 (1110xxxx 10xxxxxx 10xxxxxx)
            if (i + 2 < utf8String.length()) {
                codepoint = ((c & 0x0F) << 12) |
                           ((static_cast<uint8_t>(utf8String[i + 1]) & 0x3F) << 6) |
                           (static_cast<uint8_t>(utf8String[i + 2]) & 0x3F);
                i += 3;
            } else {
                i++; // 不正なシーケンス
            }
        } else if ((c & 0xF8) == 0xF0) {
            // 4バイト文字 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
            if (i + 3 < utf8String.length()) {
                codepoint = ((c & 0x07) << 18) |
                           ((static_cast<uint8_t>(utf8String[i + 1]) & 0x3F) << 12) |
                           ((static_cast<uint8_t>(utf8String[i + 2]) & 0x3F) << 6) |
                           (static_cast<uint8_t>(utf8String[i + 3]) & 0x3F);
                i += 4;
            } else {
                i++; // 不正なシーケンス
            }
        } else {
            // 不正なUTF-8シーケンス
            i++;
            continue;
        }
        
        if (codepoint > 0) {
            codepoints.push_back(codepoint);
        }
    }
    return codepoints;
}

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

    // UTF-8文字列をコードポイント列に変換
    std::vector<uint32_t> codepoints = Utf8ToCodepoints(text);

    // アライメントに応じたオフセットを計算
    float textWidth = 0.0f;
    for (uint32_t codepoint : codepoints) {
        const GlyphInfo* glyph = font->GetGlyph(codepoint);
        if (glyph) {
            textWidth += glyph->advance * scale;
        } else {
            textWidth += font->GetFontSize() * scale * 0.5f;
        }
    }
    
    float offsetX = 0.0f;
    switch (align) {
        case TextAlign::Center:
            offsetX = -textWidth / 2.0f;  // 中央揃え
            break;
        case TextAlign::Right:
            offsetX = -textWidth;  // 右揃え
            break;
        default:
            break;  // 左揃えはオフセット0
    }

    Texture* atlas = font->GetAtlasTexture();
    float x = position.x + offsetX;
    // position.y は行の上端を基準とする
    float y = position.y + font->GetAscent() * scale;

    // 各文字を描画
    for (uint32_t codepoint : codepoints) {
        const GlyphInfo* glyph = font->GetGlyph(codepoint);
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
    // position.y は行の上端を基準とする
    float y = position.y + font->GetAscent() * scale;

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
