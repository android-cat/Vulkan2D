/**
 * @file InputManager.cpp
 * @brief 入力管理クラスの実装
 * 
 * キーボード、マウスの入力を管理する
 * 押下状態、押した瞬間、離した瞬間を検出可能
 */

#include "Vulkan2D/Input/InputManager.h"
#include "Vulkan2D/Core/Window.h"

namespace V2D {

/**
 * @brief 入力マネージャーのコンストラクタ
 * @param window 入力を取得するウィンドウ
 * 
 * GLFWコールバックを設定してキーボード・マウス入力を受け取る
 */
InputManager::InputManager(Window* window) : m_Window(window) {
    m_GLFWWindow = window->GetHandle();
    
    // コールバック用にthisポインタを設定
    glfwSetWindowUserPointer(m_GLFWWindow, this);
    
    // 各種入力コールバックを登録
    glfwSetKeyCallback(m_GLFWWindow, KeyCallbackGLFW);
    glfwSetMouseButtonCallback(m_GLFWWindow, MouseButtonCallbackGLFW);
    glfwSetScrollCallback(m_GLFWWindow, ScrollCallbackGLFW);
    glfwSetCursorPosCallback(m_GLFWWindow, CursorPosCallbackGLFW);
    
    // 初期マウス位置を取得
    double x, y;
    glfwGetCursorPos(m_GLFWWindow, &x, &y);
    m_MousePosition = glm::vec2(static_cast<float>(x), static_cast<float>(y));
    m_PreviousMousePosition = m_MousePosition;
}

/**
 * @brief フレームごとの入力状態更新
 * 
 * 前フレームの状態を保存して、押下開始・終了を検出可能にする
 * ゲームループの最初に呼び出す
 */
void InputManager::Update() {
    m_PreviousKeys = m_CurrentKeys;
    m_PreviousMouseButtons = m_CurrentMouseButtons;
    m_PreviousMousePosition = m_MousePosition;
    m_ScrollDelta = 0.0f;  // スクロール量はフレームごとにリセット
}

/**
 * @brief キーが押されているかチェック
 * @param key チェックするキー
 * @return 押されていればtrue
 */
bool InputManager::IsKeyDown(Key key) const {
    return m_CurrentKeys[static_cast<int>(key)];
}

/**
 * @brief キーが今フレームで押されたかチェック
 * @param key チェックするキー
 * @return 今フレームで押されたらtrue
 */
bool InputManager::IsKeyPressed(Key key) const {
    int k = static_cast<int>(key);
    return m_CurrentKeys[k] && !m_PreviousKeys[k];
}

/**
 * @brief キーが今フレームで離されたかチェック
 * @param key チェックするキー
 * @return 今フレームで離されたらtrue
 */
bool InputManager::IsKeyReleased(Key key) const {
    int k = static_cast<int>(key);
    return !m_CurrentKeys[k] && m_PreviousKeys[k];
}

/**
 * @brief マウスボタンが押されているかチェック
 * @param button チェックするボタン
 * @return 押されていればtrue
 */
bool InputManager::IsMouseButtonDown(MouseButton button) const {
    return m_CurrentMouseButtons[static_cast<int>(button)];
}

/**
 * @brief マウスボタンが今フレームで押されたかチェック
 * @param button チェックするボタン
 * @return 今フレームで押されたらtrue
 */
bool InputManager::IsMouseButtonPressed(MouseButton button) const {
    int b = static_cast<int>(button);
    return m_CurrentMouseButtons[b] && !m_PreviousMouseButtons[b];
}

/**
 * @brief マウスボタンが今フレームで離されたかチェック
 * @param button チェックするボタン
 * @return 今フレームで離されたらtrue
 */
bool InputManager::IsMouseButtonReleased(MouseButton button) const {
    int b = static_cast<int>(button);
    return !m_CurrentMouseButtons[b] && m_PreviousMouseButtons[b];
}

/**
 * @brief 現在のマウス位置を取得
 * @return スクリーン座標でのマウス位置
 */
glm::vec2 InputManager::GetMousePosition() const {
    return m_MousePosition;
}

/**
 * @brief 前フレームからのマウス移動量を取得
 * @return マウスの移動量（ピクセル）
 */
glm::vec2 InputManager::GetMouseDelta() const {
    return m_MousePosition - m_PreviousMousePosition;
}

/**
 * @brief マウスホイールのスクロール量を取得
 * @return スクロール量（上方向が正）
 */
float InputManager::GetScrollDelta() const {
    return m_ScrollDelta;
}

/**
 * @brief キー入力コールバック（GLFW用、static）
 */
void InputManager::KeyCallbackGLFW(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* input = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (key >= 0 && key <= GLFW_KEY_LAST) {
        bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
        input->m_CurrentKeys[key] = pressed;
        
        // ユーザー定義コールバックを呼び出し
        if (input->m_KeyCallback) {
            input->m_KeyCallback(static_cast<Key>(key), pressed);
        }
    }
}

/**
 * @brief マウスボタンコールバック（GLFW用、static）
 */
void InputManager::MouseButtonCallbackGLFW(GLFWwindow* window, int button, int action, int mods) {
    auto* input = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST) {
        bool pressed = (action == GLFW_PRESS);
        input->m_CurrentMouseButtons[button] = pressed;
        
        // ユーザー定義コールバックを呼び出し
        if (input->m_MouseCallback) {
            input->m_MouseCallback(static_cast<MouseButton>(button), pressed);
        }
    }
}

/**
 * @brief スクロールコールバック（GLFW用、static）
 */
void InputManager::ScrollCallbackGLFW(GLFWwindow* window, double xoffset, double yoffset) {
    auto* input = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
    input->m_ScrollDelta = static_cast<float>(yoffset);
    
    // ユーザー定義コールバックを呼び出し
    if (input->m_ScrollCallback) {
        input->m_ScrollCallback(static_cast<float>(yoffset));
    }
}

/**
 * @brief カーソル位置コールバック（GLFW用、static）
 */
void InputManager::CursorPosCallbackGLFW(GLFWwindow* window, double xpos, double ypos) {
    auto* input = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
    input->m_MousePosition = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
}

} // namespace V2D
