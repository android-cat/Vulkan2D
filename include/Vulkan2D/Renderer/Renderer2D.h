#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <functional>

namespace V2D {

class VulkanContext;
class SpriteBatch;
class Camera2D;
class Window;
struct BatchStatistics;

class Renderer2D {
public:
    Renderer2D(VulkanContext* context, Window* window);
    ~Renderer2D();

    // Non-copyable
    Renderer2D(const Renderer2D&) = delete;
    Renderer2D& operator=(const Renderer2D&) = delete;

    bool BeginFrame();
    void EndFrame();

    SpriteBatch* GetSpriteBatch() { return m_SpriteBatch.get(); }
    VkCommandBuffer GetCurrentCommandBuffer() const { return m_CommandBuffers[m_CurrentFrame]; }
    uint32_t GetCurrentFrame() const { return m_CurrentFrame; }

    void SetClearColor(float r, float g, float b, float a = 1.0f);

    void OnWindowResize(int width, int height);
    
    // 描画統計情報を取得
    const BatchStatistics& GetBatchStatistics() const;

private:
    void CreateCommandBuffers();
    void CreateSyncObjects();
    void RecreateSwapChain();

    void BeginRendering(VkCommandBuffer commandBuffer);
    void EndRendering(VkCommandBuffer commandBuffer);

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    VulkanContext* m_Context;
    Window* m_Window;

    std::unique_ptr<SpriteBatch> m_SpriteBatch;

    std::vector<VkCommandBuffer> m_CommandBuffers;
    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;
    std::vector<VkFence> m_ImagesInFlight;

    uint32_t m_CurrentFrame = 0;
    uint32_t m_CurrentImageIndex = 0;
    bool m_FramebufferResized = false;

    VkClearColorValue m_ClearColor = {{0.1f, 0.1f, 0.15f, 1.0f}};
};

} // namespace V2D
