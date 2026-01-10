#pragma once

#include <string>
#include <functional>
#include <memory>

struct GLFWwindow;

namespace V2D {

struct WindowConfig {
    std::string title = "Vulkan2D Application";
    int width = 1280;
    int height = 720;
    bool resizable = true;
    bool vsync = true;
    bool fullscreen = false;
};

class Window {
public:
    Window(const WindowConfig& config);
    ~Window();

    // Non-copyable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool ShouldClose() const;
    void PollEvents();
    void Close();

    GLFWwindow* GetHandle() const { return m_Window; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    float GetAspectRatio() const { return static_cast<float>(m_Width) / static_cast<float>(m_Height); }
    bool IsMinimized() const { return m_Width == 0 || m_Height == 0; }
    bool WasResized() const { return m_FramebufferResized; }
    void ResetResizedFlag() { m_FramebufferResized = false; }

    // Callback setters
    using ResizeCallback = std::function<void(int, int)>;
    void SetResizeCallback(ResizeCallback callback) { m_ResizeCallback = callback; }

private:
    GLFWwindow* m_Window = nullptr;
    int m_Width;
    int m_Height;
    bool m_FramebufferResized = false;
    ResizeCallback m_ResizeCallback;

    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
};

} // namespace V2D
