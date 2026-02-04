/**
 * @file sprite_instanced.frag
 * @brief インスタンシング対応スプライト用フラグメントシェーダー
 * 
 * sprite.fragと同一の機能を提供
 */
#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    outColor = fragColor * texColor;
}
