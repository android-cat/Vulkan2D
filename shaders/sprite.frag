/**
 * @file sprite.frag
 * @brief スプライト用フラグメントシェーダー
 * 
 * テクスチャサンプリングと色合成を行う
 * テクスチャ色 × 頂点色 で最終色を決定
 */
#version 450

// 入力: 頂点シェーダーから補間された値
layout(location = 0) in vec4 fragColor;     // 補間済み色
layout(location = 1) in vec2 fragTexCoord;  // 補間済みUV座標

// 出力: フレームバッファへの色
layout(location = 0) out vec4 outColor;

// テクスチャサンプラー（ディスクリプタセットでバインド）
layout(binding = 0) uniform sampler2D texSampler;

void main() {
    // テクスチャから色をサンプリング
    vec4 texColor = texture(texSampler, fragTexCoord);
    
    // テクスチャ色と頂点色を乗算して最終色を決定
    // これにより白色テクスチャ+色付き頂点=色付き描画が可能
    outColor = fragColor * texColor;
}
