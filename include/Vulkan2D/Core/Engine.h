#pragma once

#include "Vulkan2D/Core/Window.h"
#include "Vulkan2D/Renderer/VulkanContext.h"
#include "Vulkan2D/Renderer/Renderer2D.h"
#include "Vulkan2D/Input/InputManager.h"
#include "Vulkan2D/Audio/AudioManager.h"
#include <memory>
#include <functional>
#include <chrono>

namespace V2D {

struct EngineConfig {
    WindowConfig windowConfig;
    bool enableValidation = true;
};

class Engine {
public:
    Engine(const EngineConfig& config);
    ~Engine();

    // Non-copyable
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // Run the engine with a custom update function
    using UpdateCallback = std::function<void(float deltaTime)>;
    using RenderCallback = std::function<void()>;
    
    void Run(UpdateCallback onUpdate, RenderCallback onRender);
    void Stop();

    // Getters
    Window* GetWindow() { return m_Window.get(); }
    VulkanContext* GetVulkanContext() { return m_VulkanContext.get(); }
    Renderer2D* GetRenderer() { return m_Renderer.get(); }
    InputManager* GetInput() { return m_Input.get(); }
    AudioManager* GetAudio() { return m_Audio.get(); }
    
    float GetDeltaTime() const { return m_DeltaTime; }
    float GetFPS() const { return m_FPS; }
    double GetTotalTime() const { return m_TotalTime; }

private:
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<VulkanContext> m_VulkanContext;
    std::unique_ptr<Renderer2D> m_Renderer;
    std::unique_ptr<InputManager> m_Input;
    std::unique_ptr<AudioManager> m_Audio;

    bool m_Running = false;
    float m_DeltaTime = 0.0f;
    float m_FPS = 0.0f;
    double m_TotalTime = 0.0;
    
    std::chrono::high_resolution_clock::time_point m_LastFrameTime;
};

} // namespace V2D
