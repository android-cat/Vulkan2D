#include "Vulkan2D/Graphics/Font.h"
#include "Vulkan2D/Renderer/VulkanContext.h"
#include "Vulkan2D/Renderer/Texture.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdexcept>
#include <vector>
#include <algorithm>
#include <cstring>
#include <codecvt>
#include <locale>

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

Font::Font(VulkanContext* context, const std::string& fontPath, uint32_t fontSize)
    : m_Context(context), m_FontSize(fontSize) {
    
    // Initialize FreeType
    if (FT_Init_FreeType(&m_FTLibrary)) {
        throw std::runtime_error("Failed to initialize FreeType library");
    }

    // Load font face
    if (FT_New_Face(m_FTLibrary, fontPath.c_str(), 0, &m_FTFace)) {
        FT_Done_FreeType(m_FTLibrary);
        throw std::runtime_error("Failed to load font: " + fontPath);
    }

    // Set font size
    FT_Set_Pixel_Sizes(m_FTFace, 0, fontSize);

    // Calculate line height
    m_LineHeight = static_cast<float>(m_FTFace->size->metrics.height >> 6);

    LoadGlyphs();
    CreateAtlas();
}

Font::~Font() {
    if (m_FTFace) {
        FT_Done_Face(m_FTFace);
    }
    if (m_FTLibrary) {
        FT_Done_FreeType(m_FTLibrary);
    }
}

void Font::LoadGlyphs() {
    // Load ASCII characters (32-126) and extended characters
    std::vector<uint32_t> characters;
    
    // Basic ASCII
    for (uint32_t c = 32; c < 127; c++) {
        characters.push_back(c);
    }
    
    // Japanese Hiragana (3040-309F)
    for (uint32_t c = 0x3040; c <= 0x309F; c++) {
        characters.push_back(c);
    }
    
    // Japanese Katakana (30A0-30FF)
    for (uint32_t c = 0x30A0; c <= 0x30FF; c++) {
        characters.push_back(c);
    }
    
    // Common Kanji (4E00-9FFF) - load a subset for performance
    for (uint32_t c = 0x4E00; c <= 0x4FFF; c++) {
        characters.push_back(c);
    }

    // Calculate atlas size
    uint32_t maxWidth = 0;
    uint32_t maxHeight = 0;
    uint32_t totalWidth = 0;
    
    for (uint32_t c : characters) {
        if (FT_Load_Char(m_FTFace, c, FT_LOAD_RENDER)) {
            continue;
        }
        
        FT_GlyphSlot g = m_FTFace->glyph;
        totalWidth += g->bitmap.width + 2;
        maxHeight = std::max(maxHeight, g->bitmap.rows);
    }

    // Create atlas dimensions (roughly square)
    uint32_t atlasSize = 512;
    while (atlasSize * atlasSize < totalWidth * (maxHeight + 2)) {
        atlasSize *= 2;
    }
    m_AtlasWidth = atlasSize;
    m_AtlasHeight = atlasSize;
}

void Font::CreateAtlas() {
    std::vector<uint8_t> atlasData(m_AtlasWidth * m_AtlasHeight * 4, 0);
    
    uint32_t penX = 0;
    uint32_t penY = 0;
    uint32_t rowHeight = 0;

    // Characters to load
    std::vector<uint32_t> characters;
    for (uint32_t c = 32; c < 127; c++) {
        characters.push_back(c);
    }
    for (uint32_t c = 0x3040; c <= 0x309F; c++) {
        characters.push_back(c);
    }
    for (uint32_t c = 0x30A0; c <= 0x30FF; c++) {
        characters.push_back(c);
    }
    for (uint32_t c = 0x4E00; c <= 0x4FFF; c++) {
        characters.push_back(c);
    }

    for (uint32_t c : characters) {
        if (FT_Load_Char(m_FTFace, c, FT_LOAD_RENDER)) {
            continue;
        }

        FT_GlyphSlot g = m_FTFace->glyph;
        
        // Check if we need to move to next row
        if (penX + g->bitmap.width + 2 >= m_AtlasWidth) {
            penX = 0;
            penY += rowHeight + 2;
            rowHeight = 0;
        }
        
        // Check if atlas is full
        if (penY + g->bitmap.rows >= m_AtlasHeight) {
            break;
        }

        // Copy glyph bitmap to atlas (convert to RGBA)
        for (uint32_t y = 0; y < g->bitmap.rows; y++) {
            for (uint32_t x = 0; x < g->bitmap.width; x++) {
                uint32_t atlasIdx = ((penY + y) * m_AtlasWidth + (penX + x)) * 4;
                uint8_t value = g->bitmap.buffer[y * g->bitmap.width + x];
                atlasData[atlasIdx + 0] = 255;  // R
                atlasData[atlasIdx + 1] = 255;  // G
                atlasData[atlasIdx + 2] = 255;  // B
                atlasData[atlasIdx + 3] = value; // A
            }
        }

        // Store glyph info
        GlyphInfo info;
        info.size = glm::ivec2(g->bitmap.width, g->bitmap.rows);
        info.bearing = glm::ivec2(g->bitmap_left, g->bitmap_top);
        info.advance = static_cast<uint32_t>(g->advance.x >> 6);
        info.uvMin = glm::vec2(
            static_cast<float>(penX) / m_AtlasWidth,
            static_cast<float>(penY) / m_AtlasHeight
        );
        info.uvMax = glm::vec2(
            static_cast<float>(penX + g->bitmap.width) / m_AtlasWidth,
            static_cast<float>(penY + g->bitmap.rows) / m_AtlasHeight
        );
        
        m_Glyphs[c] = info;

        penX += g->bitmap.width + 2;
        rowHeight = std::max(rowHeight, g->bitmap.rows);
    }

    // Create texture from atlas
    m_AtlasTexture = std::make_unique<Texture>(m_Context, m_AtlasWidth, m_AtlasHeight, atlasData.data());
}

const GlyphInfo* Font::GetGlyph(uint32_t codepoint) const {
    auto it = m_Glyphs.find(codepoint);
    if (it != m_Glyphs.end()) {
        return &it->second;
    }
    return nullptr;
}

glm::vec2 Font::MeasureText(const std::string& text) const {
    float width = 0.0f;
    float maxHeight = 0.0f;

    // UTF-8文字列をコードポイント列に変換
    std::vector<uint32_t> codepoints = Utf8ToCodepoints(text);
    
    for (uint32_t codepoint : codepoints) {
        const GlyphInfo* glyph = GetGlyph(codepoint);
        if (glyph) {
            width += glyph->advance;
            maxHeight = std::max(maxHeight, static_cast<float>(glyph->size.y));
        }
    }

    return glm::vec2(width, maxHeight);
}

glm::vec2 Font::MeasureText(const std::wstring& text) const {
    float width = 0.0f;
    float maxHeight = 0.0f;

    for (wchar_t c : text) {
        const GlyphInfo* glyph = GetGlyph(static_cast<uint32_t>(c));
        if (glyph) {
            width += glyph->advance;
            maxHeight = std::max(maxHeight, static_cast<float>(glyph->size.y));
        }
    }

    return glm::vec2(width, maxHeight);
}

} // namespace V2D
