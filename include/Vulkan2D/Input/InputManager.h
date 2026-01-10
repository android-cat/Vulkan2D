#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <array>
#include <functional>

namespace V2D {

class Window;

enum class Key {
    // Letters
    A = GLFW_KEY_A, B = GLFW_KEY_B, C = GLFW_KEY_C, D = GLFW_KEY_D,
    E = GLFW_KEY_E, F = GLFW_KEY_F, G = GLFW_KEY_G, H = GLFW_KEY_H,
    I = GLFW_KEY_I, J = GLFW_KEY_J, K = GLFW_KEY_K, L = GLFW_KEY_L,
    M = GLFW_KEY_M, N = GLFW_KEY_N, O = GLFW_KEY_O, P = GLFW_KEY_P,
    Q = GLFW_KEY_Q, R = GLFW_KEY_R, S = GLFW_KEY_S, T = GLFW_KEY_T,
    U = GLFW_KEY_U, V = GLFW_KEY_V, W = GLFW_KEY_W, X = GLFW_KEY_X,
    Y = GLFW_KEY_Y, Z = GLFW_KEY_Z,
    
    // Numbers
    Num0 = GLFW_KEY_0, Num1 = GLFW_KEY_1, Num2 = GLFW_KEY_2, Num3 = GLFW_KEY_3,
    Num4 = GLFW_KEY_4, Num5 = GLFW_KEY_5, Num6 = GLFW_KEY_6, Num7 = GLFW_KEY_7,
    Num8 = GLFW_KEY_8, Num9 = GLFW_KEY_9,
    
    // Function keys
    F1 = GLFW_KEY_F1, F2 = GLFW_KEY_F2, F3 = GLFW_KEY_F3, F4 = GLFW_KEY_F4,
    F5 = GLFW_KEY_F5, F6 = GLFW_KEY_F6, F7 = GLFW_KEY_F7, F8 = GLFW_KEY_F8,
    F9 = GLFW_KEY_F9, F10 = GLFW_KEY_F10, F11 = GLFW_KEY_F11, F12 = GLFW_KEY_F12,
    
    // Special keys
    Space = GLFW_KEY_SPACE,
    Enter = GLFW_KEY_ENTER,
    Escape = GLFW_KEY_ESCAPE,
    Tab = GLFW_KEY_TAB,
    Backspace = GLFW_KEY_BACKSPACE,
    Delete = GLFW_KEY_DELETE,
    Insert = GLFW_KEY_INSERT,
    Home = GLFW_KEY_HOME,
    End = GLFW_KEY_END,
    PageUp = GLFW_KEY_PAGE_UP,
    PageDown = GLFW_KEY_PAGE_DOWN,
    
    // Arrow keys
    Up = GLFW_KEY_UP,
    Down = GLFW_KEY_DOWN,
    Left = GLFW_KEY_LEFT,
    Right = GLFW_KEY_RIGHT,
    
    // Modifiers
    LeftShift = GLFW_KEY_LEFT_SHIFT,
    RightShift = GLFW_KEY_RIGHT_SHIFT,
    LeftControl = GLFW_KEY_LEFT_CONTROL,
    RightControl = GLFW_KEY_RIGHT_CONTROL,
    LeftAlt = GLFW_KEY_LEFT_ALT,
    RightAlt = GLFW_KEY_RIGHT_ALT
};

enum class MouseButton {
    Left = GLFW_MOUSE_BUTTON_LEFT,
    Right = GLFW_MOUSE_BUTTON_RIGHT,
    Middle = GLFW_MOUSE_BUTTON_MIDDLE
};

class InputManager {
public:
    InputManager(Window* window);
    
    void Update();
    
    // Keyboard
    bool IsKeyDown(Key key) const;
    bool IsKeyPressed(Key key) const;  // Just pressed this frame
    bool IsKeyReleased(Key key) const; // Just released this frame
    
    // Mouse
    bool IsMouseButtonDown(MouseButton button) const;
    bool IsMouseButtonPressed(MouseButton button) const;
    bool IsMouseButtonReleased(MouseButton button) const;
    
    glm::vec2 GetMousePosition() const;
    glm::vec2 GetMouseDelta() const;
    float GetScrollDelta() const;
    
    // Callbacks
    using KeyCallback = std::function<void(Key, bool)>;
    using MouseCallback = std::function<void(MouseButton, bool)>;
    using ScrollCallback = std::function<void(float)>;
    
    void SetKeyCallback(KeyCallback callback) { m_KeyCallback = callback; }
    void SetMouseCallback(MouseCallback callback) { m_MouseCallback = callback; }
    void SetScrollCallback(ScrollCallback callback) { m_ScrollCallback = callback; }

private:
    static void KeyCallbackGLFW(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallbackGLFW(GLFWwindow* window, int button, int action, int mods);
    static void ScrollCallbackGLFW(GLFWwindow* window, double xoffset, double yoffset);
    static void CursorPosCallbackGLFW(GLFWwindow* window, double xpos, double ypos);
    
    Window* m_Window;
    GLFWwindow* m_GLFWWindow;
    
    std::array<bool, GLFW_KEY_LAST + 1> m_CurrentKeys{};
    std::array<bool, GLFW_KEY_LAST + 1> m_PreviousKeys{};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> m_CurrentMouseButtons{};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> m_PreviousMouseButtons{};
    
    glm::vec2 m_MousePosition{0.0f};
    glm::vec2 m_PreviousMousePosition{0.0f};
    float m_ScrollDelta = 0.0f;
    
    KeyCallback m_KeyCallback;
    MouseCallback m_MouseCallback;
    ScrollCallback m_ScrollCallback;
};

} // namespace V2D
