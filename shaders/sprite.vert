/**
 * @file sprite.vert
 * @brief スプライト用頂点シェーダー
 * 
 * 2Dスプライトの変換を行う
 * プッシュ定数でビュー投影行列を受け取る
 */
#version 450

// 入力: 頂点属性
layout(location = 0) in vec3 inPosition;   // ワールド座標位置
layout(location = 1) in vec4 inColor;      // 頂点色（RGBA）
layout(location = 2) in vec2 inTexCoord;   // テクスチャ座標（UV）

// 出力: フラグメントシェーダーへ
layout(location = 0) out vec4 fragColor;    // 補間される色
layout(location = 1) out vec2 fragTexCoord; // 補間されるUV座標

// プッシュ定数: 每フレーム更新される小さなデータ
layout(push_constant) uniform PushConstants {
    mat4 viewProjection;  // ビュー投影行列
} pushConstants;

void main() {
    // ワールド座標をクリップ座標に変換
    gl_Position = pushConstants.viewProjection * vec4(inPosition, 1.0);
    
    // 色とUV座標をフラグメントシェーダーに渡す
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
