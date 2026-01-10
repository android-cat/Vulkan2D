#pragma once

#include <string>
#include <map>
#include <memory>
#include <glm/glm.hpp>

struct FT_LibraryRec_;
struct FT_FaceRec_;
typedef FT_LibraryRec_* FT_Library;
typedef FT_FaceRec_* FT_Face;

namespace V2D {

class VulkanContext;
class Texture;

struct GlyphInfo {
    glm::ivec2 size;       // Size of glyph
    glm::ivec2 bearing;    // Offset from baseline to left/top of glyph
    uint32_t advance;      // Horizontal offset to advance to next glyph
    glm::vec2 uvMin;       // UV coordinates in atlas
    glm::vec2 uvMax;
};

class Font {
public:
    Font(VulkanContext* context, const std::string& fontPath, uint32_t fontSize);
    ~Font();

    // Non-copyable
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    const GlyphInfo* GetGlyph(uint32_t codepoint) const;
    Texture* GetAtlasTexture() const { return m_AtlasTexture.get(); }
    uint32_t GetFontSize() const { return m_FontSize; }
    float GetLineHeight() const { return m_LineHeight; }

    // Calculate text dimensions
    glm::vec2 MeasureText(const std::string& text) const;
    glm::vec2 MeasureText(const std::wstring& text) const;

private:
    void LoadGlyphs();
    void CreateAtlas();

    VulkanContext* m_Context;
    FT_Library m_FTLibrary = nullptr;
    FT_Face m_FTFace = nullptr;
    
    std::map<uint32_t, GlyphInfo> m_Glyphs;
    std::unique_ptr<Texture> m_AtlasTexture;
    
    uint32_t m_FontSize;
    float m_LineHeight;
    uint32_t m_AtlasWidth = 0;
    uint32_t m_AtlasHeight = 0;
};

} // namespace V2D
