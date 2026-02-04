/**
 * @file sprite_instanced.vert
 * @brief インスタンシング対応スプライト用頂点シェーダー
 * 
 * GPUインスタンシングを使用して同一テクスチャの
 * スプライトを一括描画する最適化バージョン
 */
#version 450

// 頂点入力（スプライトの4頂点共通のクワッドデータ）
layout(location = 0) in vec2 inQuadPos;      // クワッドの相対位置 (-0.5 to 0.5)
layout(location = 1) in vec2 inQuadTexCoord; // クワッドのUV座標 (0 to 1)

// インスタンス入力（各スプライト固有のデータ）
layout(location = 2) in vec2 inPosition;     // ワールド座標位置
layout(location = 3) in vec2 inSize;         // スプライトサイズ
layout(location = 4) in vec2 inOrigin;       // 回転原点
layout(location = 5) in float inRotation;    // 回転角（ラジアン）
layout(location = 6) in vec4 inColor;        // 色
layout(location = 7) in vec4 inUVRect;       // UV矩形 (minU, minV, maxU, maxV)

// 出力
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

// プッシュ定数
layout(push_constant) uniform PushConstants {
    mat4 viewProjection;
} pushConstants;

void main() {
    // クワッド頂点を原点中心に変換 (-0.5 ~ 0.5を0 ~ sizeに)
    vec2 localPos = (inQuadPos + 0.5) * inSize - inOrigin;
    
    // 回転を適用
    float cosR = cos(inRotation);
    float sinR = sin(inRotation);
    vec2 rotatedPos = vec2(
        localPos.x * cosR - localPos.y * sinR,
        localPos.x * sinR + localPos.y * cosR
    );
    
    // ワールド位置に変換
    vec2 worldPos = rotatedPos + inPosition;
    
    // クリップ座標に変換
    gl_Position = pushConstants.viewProjection * vec4(worldPos, 0.0, 1.0);
    
    // UV座標を計算（UVRectを使用）
    fragTexCoord = mix(inUVRect.xy, inUVRect.zw, inQuadTexCoord);
    
    // 色を渡す
    fragColor = inColor;
}
