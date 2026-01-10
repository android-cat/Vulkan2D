/**
 * @file Window.cpp
 * @brief ウィンドウ管理クラスの実装
 * 
 * GLFWを使用してウィンドウの作成・管理を行う
 * Vulkanサーフェス用のウィンドウとして機能する
 */

#include "Vulkan2D/Core/Window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace V2D {

/**
 * @brief ウィンドウのコンストラクタ
 * @param config ウィンドウ設定（サイズ、タイトル、フルスクリーンなど）
 * 
 * GLFWの初期化とウィンドウ作成を行う
 * GLFW_CLIENT_APIをGLFW_NO_APIに設定してVulkan用ウィンドウにする
 */
Window::Window(const WindowConfig& config) 
    : m_Width(config.width), m_Height(config.height) {
    
    // GLFWを初期化
    if (!glfwInit()) {
        throw std::runtime_error("GLFWの初期化に失敗しました");
    }

    // Vulkan用ウィンドウの設定（OpenGLコンテキストなし）
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // リサイズ可能かどうかを設定
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

    // フルスクリーンモードの場合、プライマリモニターを使用
    GLFWmonitor* monitor = nullptr;
    if (config.fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        m_Width = mode->width;
        m_Height = mode->height;
    }

    // ウィンドウを作成
    m_Window = glfwCreateWindow(m_Width, m_Height, config.title.c_str(), monitor, nullptr);
    if (!m_Window) {
        glfwTerminate();
        throw std::runtime_error("GLFWウィンドウの作成に失敗しました");
    }

    // コールバック用にthisポインタを設定
    glfwSetWindowUserPointer(m_Window, this);
    // フレームバッファリサイズコールバックを設定
    glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallback);
}

/**
 * @brief ウィンドウのデストラクタ
 * 
 * ウィンドウを破棄し、GLFWを終了する
 */
Window::~Window() {
    if (m_Window) {
        glfwDestroyWindow(m_Window);
    }
    glfwTerminate();
}

/**
 * @brief ウィンドウを閉じるべきかチェック
 * @return 閉じるべきならtrue
 */
bool Window::ShouldClose() const {
    return glfwWindowShouldClose(m_Window);
}

/**
 * @brief ウィンドウイベントをポーリング
 * 
 * キーボード、マウス、ウィンドウイベントを処理する
 */
void Window::PollEvents() {
    glfwPollEvents();
}

/**
 * @brief ウィンドウを閉じるようマーク
 */
void Window::Close() {
    glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
}

/**
 * @brief フレームバッファリサイズコールバック（static）
 * @param window GLFWウィンドウハンドル
 * @param width 新しい幅
 * @param height 新しい高さ
 * 
 * ウィンドウサイズが変更されたときに呼ばれる
 * スワップチェーンの再作成をトリガーする
 */
void Window::FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto* app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    app->m_FramebufferResized = true;
    app->m_Width = width;
    app->m_Height = height;
    
    // ユーザー定義のリサイズコールバックを呼び出し
    if (app->m_ResizeCallback) {
        app->m_ResizeCallback(width, height);
    }
}

} // namespace V2D
