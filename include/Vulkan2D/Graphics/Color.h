#pragma once

#include <glm/glm.hpp>
#include <string>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace V2D {

/**
 * @brief 色のユーティリティクラス
 * 
 * RGB(0-255)、RGBA(0-255)、HTMLカラーコードなど
 * さまざまな形式から色を作成できます。
 */
class Color {
public:
    // ========================================
    // コンストラクタ
    // ========================================

    /**
     * @brief デフォルトコンストラクタ（白色）
     */
    Color() : m_Color(1.0f, 1.0f, 1.0f, 1.0f) {}

    /**
     * @brief glm::vec4から作成
     * @param color 色（0.0-1.0）
     */
    Color(const glm::vec4& color) : m_Color(color) {}

    /**
     * @brief glm::vec3から作成（アルファは1.0）
     * @param color RGB色（0.0-1.0）
     */
    Color(const glm::vec3& color) : m_Color(color.r, color.g, color.b, 1.0f) {}

    // ========================================
    // 静的ファクトリメソッド
    // ========================================

    /**
     * @brief RGB値から色を作成（0-255）
     * @param r 赤成分（0-255）
     * @param g 緑成分（0-255）
     * @param b 青成分（0-255）
     * @return Color オブジェクト
     */
    static Color RGB(uint8_t r, uint8_t g, uint8_t b) {
        return Color(glm::vec4(
            r / 255.0f,
            g / 255.0f,
            b / 255.0f,
            1.0f
        ));
    }

    /**
     * @brief RGB値から色を作成（int版、0-255）
     * @param r 赤成分（0-255）
     * @param g 緑成分（0-255）
     * @param b 青成分（0-255）
     * @return Color オブジェクト
     */
    static Color RGB(int r, int g, int b) {
        return RGB(
            static_cast<uint8_t>(std::clamp(r, 0, 255)),
            static_cast<uint8_t>(std::clamp(g, 0, 255)),
            static_cast<uint8_t>(std::clamp(b, 0, 255))
        );
    }

    /**
     * @brief RGBA値から色を作成（0-255）
     * @param r 赤成分（0-255）
     * @param g 緑成分（0-255）
     * @param b 青成分（0-255）
     * @param a アルファ成分（0-255）
     * @return Color オブジェクト
     */
    static Color RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        return Color(glm::vec4(
            r / 255.0f,
            g / 255.0f,
            b / 255.0f,
            a / 255.0f
        ));
    }

    /**
     * @brief RGBA値から色を作成（int版、0-255）
     */
    static Color RGBA(int r, int g, int b, int a) {
        return RGBA(
            static_cast<uint8_t>(std::clamp(r, 0, 255)),
            static_cast<uint8_t>(std::clamp(g, 0, 255)),
            static_cast<uint8_t>(std::clamp(b, 0, 255)),
            static_cast<uint8_t>(std::clamp(a, 0, 255))
        );
    }

    /**
     * @brief RGB値から色を作成（アルファ指定、0.0-1.0）
     * @param r 赤成分（0-255）
     * @param g 緑成分（0-255）
     * @param b 青成分（0-255）
     * @param alpha アルファ値（0.0-1.0）
     * @return Color オブジェクト
     */
    static Color RGBAlpha(uint8_t r, uint8_t g, uint8_t b, float alpha) {
        return Color(glm::vec4(
            r / 255.0f,
            g / 255.0f,
            b / 255.0f,
            alpha
        ));
    }

    /**
     * @brief HTMLカラーコードから色を作成
     * @param hex カラーコード（"#RRGGBB" または "#RRGGBBAA" 形式、#は省略可）
     * @return Color オブジェクト
     * 
     * 使用例:
     *   Color::Hex("#FF0000")    // 赤
     *   Color::Hex("00FF00")     // 緑（#省略）
     *   Color::Hex("#0000FFFF")  // 青（不透明）
     *   Color::Hex("#FFFFFF80")  // 白（半透明）
     */
    static Color Hex(const std::string& hex) {
        std::string h = hex;
        
        // #を削除
        if (!h.empty() && h[0] == '#') {
            h = h.substr(1);
        }
        
        // デフォルト値
        uint8_t r = 255, g = 255, b = 255, a = 255;
        
        if (h.length() >= 6) {
            r = static_cast<uint8_t>(std::stoi(h.substr(0, 2), nullptr, 16));
            g = static_cast<uint8_t>(std::stoi(h.substr(2, 2), nullptr, 16));
            b = static_cast<uint8_t>(std::stoi(h.substr(4, 2), nullptr, 16));
        }
        
        if (h.length() >= 8) {
            a = static_cast<uint8_t>(std::stoi(h.substr(6, 2), nullptr, 16));
        }
        
        return RGBA(r, g, b, a);
    }

    /**
     * @brief HSV値から色を作成
     * @param h 色相（0.0-360.0度）
     * @param s 彩度（0.0-1.0）
     * @param v 明度（0.0-1.0）
     * @return Color オブジェクト
     */
    static Color HSV(float h, float s, float v) {
        // 色相を0-360の範囲に正規化
        h = std::fmod(h, 360.0f);
        if (h < 0) h += 360.0f;
        
        float c = v * s;
        float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;
        
        float r, g, b;
        
        if (h < 60.0f) {
            r = c; g = x; b = 0;
        } else if (h < 120.0f) {
            r = x; g = c; b = 0;
        } else if (h < 180.0f) {
            r = 0; g = c; b = x;
        } else if (h < 240.0f) {
            r = 0; g = x; b = c;
        } else if (h < 300.0f) {
            r = x; g = 0; b = c;
        } else {
            r = c; g = 0; b = x;
        }
        
        return Color(glm::vec4(r + m, g + m, b + m, 1.0f));
    }

    /**
     * @brief HSVA値から色を作成（アルファ付きHSV）
     */
    static Color HSVA(float h, float s, float v, float a) {
        Color c = HSV(h, s, v);
        c.m_Color.a = a;
        return c;
    }

    // ========================================
    // 定義済みカラー（よく使う色）
    // ========================================
    
    static Color White()       { return Color(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); }
    static Color Black()       { return Color(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)); }
    static Color Red()         { return Color(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); }
    static Color Green()       { return Color(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)); }
    static Color Blue()        { return Color(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)); }
    static Color Yellow()      { return Color(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)); }
    static Color Cyan()        { return Color(glm::vec4(0.0f, 1.0f, 1.0f, 1.0f)); }
    static Color Magenta()     { return Color(glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)); }
    static Color Orange()      { return Color(glm::vec4(1.0f, 0.5f, 0.0f, 1.0f)); }
    static Color Purple()      { return Color(glm::vec4(0.5f, 0.0f, 1.0f, 1.0f)); }
    static Color Pink()        { return Color(glm::vec4(1.0f, 0.75f, 0.8f, 1.0f)); }
    static Color Gray()        { return Color(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)); }
    static Color LightGray()   { return Color(glm::vec4(0.75f, 0.75f, 0.75f, 1.0f)); }
    static Color DarkGray()    { return Color(glm::vec4(0.25f, 0.25f, 0.25f, 1.0f)); }
    static Color Transparent() { return Color(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)); }
    
    // ゲームでよく使う色
    static Color Gold()        { return RGB(255, 215, 0); }
    static Color Silver()      { return RGB(192, 192, 192); }
    static Color Bronze()      { return RGB(205, 127, 50); }
    static Color SkyBlue()     { return RGB(135, 206, 235); }
    static Color ForestGreen() { return RGB(34, 139, 34); }
    static Color Crimson()     { return RGB(220, 20, 60); }
    static Color Navy()        { return RGB(0, 0, 128); }
    static Color Coral()       { return RGB(255, 127, 80); }
    static Color Turquoise()   { return RGB(64, 224, 208); }

    // ========================================
    // 色の操作
    // ========================================

    /**
     * @brief アルファ値を変更した新しい色を返す
     * @param alpha 新しいアルファ値（0.0-1.0）
     * @return 新しいColorオブジェクト
     */
    Color WithAlpha(float alpha) const {
        return Color(glm::vec4(m_Color.r, m_Color.g, m_Color.b, alpha));
    }

    /**
     * @brief 明るくした色を返す
     * @param amount 明るさの増加量（0.0-1.0）
     * @return 新しいColorオブジェクト
     */
    Color Lighten(float amount) const {
        return Color(glm::vec4(
            std::min(m_Color.r + amount, 1.0f),
            std::min(m_Color.g + amount, 1.0f),
            std::min(m_Color.b + amount, 1.0f),
            m_Color.a
        ));
    }

    /**
     * @brief 暗くした色を返す
     * @param amount 暗さの増加量（0.0-1.0）
     * @return 新しいColorオブジェクト
     */
    Color Darken(float amount) const {
        return Color(glm::vec4(
            std::max(m_Color.r - amount, 0.0f),
            std::max(m_Color.g - amount, 0.0f),
            std::max(m_Color.b - amount, 0.0f),
            m_Color.a
        ));
    }

    /**
     * @brief 2つの色を混ぜる
     * @param other もう一つの色
     * @param t 混合比率（0.0=this, 1.0=other）
     * @return 混合されたColorオブジェクト
     */
    Color Lerp(const Color& other, float t) const {
        return Color(glm::mix(m_Color, other.m_Color, t));
    }

    // ========================================
    // 変換・アクセス
    // ========================================

    /**
     * @brief glm::vec4への暗黙変換
     */
    operator glm::vec4() const { return m_Color; }

    /**
     * @brief glm::vec4を取得
     */
    glm::vec4 ToVec4() const { return m_Color; }

    /**
     * @brief glm::vec3を取得（アルファなし）
     */
    glm::vec3 ToVec3() const { return glm::vec3(m_Color.r, m_Color.g, m_Color.b); }

    /**
     * @brief 各成分へのアクセス（0.0-1.0）
     */
    float R() const { return m_Color.r; }
    float G() const { return m_Color.g; }
    float B() const { return m_Color.b; }
    float A() const { return m_Color.a; }

    /**
     * @brief 各成分へのアクセス（0-255）
     */
    uint8_t R255() const { return static_cast<uint8_t>(m_Color.r * 255.0f); }
    uint8_t G255() const { return static_cast<uint8_t>(m_Color.g * 255.0f); }
    uint8_t B255() const { return static_cast<uint8_t>(m_Color.b * 255.0f); }
    uint8_t A255() const { return static_cast<uint8_t>(m_Color.a * 255.0f); }

    /**
     * @brief HTMLカラーコードに変換
     * @param includeAlpha アルファ成分を含めるか
     * @return "#RRGGBB" または "#RRGGBBAA" 形式の文字列
     */
    std::string ToHex(bool includeAlpha = false) const {
        char buf[10];
        if (includeAlpha) {
            snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", R255(), G255(), B255(), A255());
        } else {
            snprintf(buf, sizeof(buf), "#%02X%02X%02X", R255(), G255(), B255());
        }
        return std::string(buf);
    }

private:
    glm::vec4 m_Color;
};

} // namespace V2D
