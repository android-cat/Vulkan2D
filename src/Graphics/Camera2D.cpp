/**
 * @file Camera2D.cpp
 * @brief 2Dカメラクラスの実装
 * 
 * 2Dゲーム用の正射影カメラを提供する
 * 位置、回転、ズームの操作が可能
 */

#include "Vulkan2D/Graphics/Camera2D.h"

namespace V2D {

/**
 * @brief カメラのコンストラクタ
 * @param width ビューポートの幅
 * @param height ビューポートの高さ
 * 
 * ワールド座標系の中心が画面中央になるよう初期化
 */
Camera2D::Camera2D(float width, float height)
    : m_ViewportWidth(width), m_ViewportHeight(height) {
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

/**
 * @brief カメラの位置を設定
 * @param position 新しい位置（ワールド座標）
 */
void Camera2D::SetPosition(const glm::vec2& position) {
    m_Position = position;
    m_ViewDirty = true;  // ビュー行列の再計算が必要
}

/**
 * @brief カメラの回転を設定
 * @param rotation 回転角度（ラジアン）
 */
void Camera2D::SetRotation(float rotation) {
    m_Rotation = rotation;
    m_ViewDirty = true;
}

/**
 * @brief カメラのズーム倍率を設定
 * @param zoom ズーム倍率（1.0が標準、大きいほどズームイン）
 */
void Camera2D::SetZoom(float zoom) {
    m_Zoom = zoom;
    // 最小ズーム倍率を0.1に制限
    if (m_Zoom < 0.1f) m_Zoom = 0.1f;
    m_ViewDirty = true;
}

/**
 * @brief ビューポートサイズを設定
 * @param width 新しい幅
 * @param height 新しい高さ
 * 
 * ウィンドウリサイズ時に呼び出す
 */
void Camera2D::SetViewport(float width, float height) {
    m_ViewportWidth = width;
    m_ViewportHeight = height;
    m_ProjectionDirty = true;  // 投影行列の再計算が必要
}

/**
 * @brief ビュー行列を取得
 * @return ビュー行列（必要に応じて再計算される）
 */
const glm::mat4& Camera2D::GetViewMatrix() const {
    if (m_ViewDirty) {
        const_cast<Camera2D*>(this)->UpdateViewMatrix();
    }
    return m_ViewMatrix;
}

/**
 * @brief 投影行列を取得
 * @return 投影行列（必要に応じて再計算される）
 */
const glm::mat4& Camera2D::GetProjectionMatrix() const {
    if (m_ProjectionDirty) {
        const_cast<Camera2D*>(this)->UpdateProjectionMatrix();
    }
    return m_ProjectionMatrix;
}

/**
 * @brief ビュー投影行列を取得
 * @return 投影行列 × ビュー行列
 */
glm::mat4 Camera2D::GetViewProjectionMatrix() const {
    return GetProjectionMatrix() * GetViewMatrix();
}

/**
 * @brief カメラを相対移動
 * @param delta 移動量（ワールド座標）
 */
void Camera2D::Move(const glm::vec2& delta) {
    m_Position += delta;
    m_ViewDirty = true;
}

/**
 * @brief カメラを相対回転
 * @param delta 回転量（ラジアン）
 */
void Camera2D::Rotate(float delta) {
    m_Rotation += delta;
    m_ViewDirty = true;
}

/**
 * @brief ズーム倍率を相対変更
 * @param delta ズームの変化量
 */
void Camera2D::Zoom(float delta) {
    m_Zoom += delta;
    if (m_Zoom < 0.1f) m_Zoom = 0.1f;
    m_ViewDirty = true;
}

/**
 * @brief スクリーン座標をワールド座標に変換
 * @param screenPos スクリーン座標（ピクセル）
 * @return ワールド座標
 * 
 * マウス位置からゲーム内座標を取得するのに使用
 */
glm::vec2 Camera2D::ScreenToWorld(const glm::vec2& screenPos) const {
    // スクリーン座標を正規化デバイス座標（-1〜1）に変換
    glm::vec2 normalized;
    normalized.x = (screenPos.x / m_ViewportWidth) * 2.0f - 1.0f;
    normalized.y = (screenPos.y / m_ViewportHeight) * 2.0f - 1.0f;
    
    // ビュー投影行列の逆行列を使ってワールド座標に変換
    glm::mat4 invVP = glm::inverse(GetViewProjectionMatrix());
    glm::vec4 worldPos = invVP * glm::vec4(normalized, 0.0f, 1.0f);
    
    return glm::vec2(worldPos.x, worldPos.y);
}

/**
 * @brief ワールド座標をスクリーン座標に変換
 * @param worldPos ワールド座標
 * @return スクリーン座標（ピクセル）
 */
glm::vec2 Camera2D::WorldToScreen(const glm::vec2& worldPos) const {
    // ビュー投影行列でクリップ座標に変換
    glm::vec4 clipPos = GetViewProjectionMatrix() * glm::vec4(worldPos, 0.0f, 1.0f);
    
    // クリップ座標をスクリーン座標に変換
    glm::vec2 screenPos;
    screenPos.x = ((clipPos.x + 1.0f) / 2.0f) * m_ViewportWidth;
    screenPos.y = ((clipPos.y + 1.0f) / 2.0f) * m_ViewportHeight;
    
    return screenPos;
}

/**
 * @brief ビュー行列を更新
 * 
 * ズーム → 回転 → 移動の順で変換を適用
 * カメラの移動は逆方向に適用（カメラが右に動く = ワールドが左に動く）
 */
void Camera2D::UpdateViewMatrix() {
    glm::mat4 transform = glm::mat4(1.0f);
    
    // ズームを適用
    transform = glm::scale(transform, glm::vec3(m_Zoom, m_Zoom, 1.0f));
    
    // 回転を適用
    transform = glm::rotate(transform, m_Rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    
    // 移動を適用（ビュー行列なので逆方向）
    transform = glm::translate(transform, glm::vec3(-m_Position, 0.0f));
    
    m_ViewMatrix = transform;
    m_ViewDirty = false;
}

/**
 * @brief 投影行列を更新
 * 
 * 正射影行列を作成する
 * ワールド座標系: 左上が(0,0)、Y+が下
 * Vulkan NDC: 上端が-1、下端が+1（Y+が下）
 */
void Camera2D::UpdateProjectionMatrix() {
     // 2D用の正射影行列を作成
     // 正射影行列（left, right, bottom, top, near, far）
     // 左上原点・ピクセル座標に合わせる（Y+は下方向）
     m_ProjectionMatrix = glm::ortho(0.0f, m_ViewportWidth, 0.0f, m_ViewportHeight, -1.0f, 1.0f);
    
    m_ProjectionDirty = false;
}

} // namespace V2D
