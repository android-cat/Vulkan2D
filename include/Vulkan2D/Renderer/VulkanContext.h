#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <string>
#include <memory>

struct GLFWwindow;

namespace V2D {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool IsComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();

    // Non-copyable
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    void Initialize(GLFWwindow* window, bool enableValidation = true);
    void Cleanup();

    void CreateSwapChain(int width, int height);
    void RecreateSwapChain(int width, int height);
    void CleanupSwapChain();

    // Getters
    VkInstance GetInstance() const { return m_Instance; }
    VkDevice GetDevice() const { return m_Device; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
    VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
    VkQueue GetPresentQueue() const { return m_PresentQueue; }
    VkSurfaceKHR GetSurface() const { return m_Surface; }
    VkSwapchainKHR GetSwapChain() const { return m_SwapChain; }
    VkFormat GetSwapChainImageFormat() const { return m_SwapChainImageFormat; }
    VkExtent2D GetSwapChainExtent() const { return m_SwapChainExtent; }
    const std::vector<VkImageView>& GetSwapChainImageViews() const { return m_SwapChainImageViews; }
    const std::vector<VkImage>& GetSwapChainImages() const { return m_SwapChainImages; }
    uint32_t GetImageCount() const { return static_cast<uint32_t>(m_SwapChainImages.size()); }
    VkCommandPool GetCommandPool() const { return m_CommandPool; }
    QueueFamilyIndices GetQueueFamilies() const { return m_QueueFamilyIndices; }

    // Helper functions
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    VkCommandBuffer BeginSingleTimeCommands() const;
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;
    
    void WaitIdle() const { vkDeviceWaitIdle(m_Device); }

private:
    void CreateInstance(bool enableValidation);
    void SetupDebugMessenger();
    void CreateSurface(GLFWwindow* window);
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateCommandPool();

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;
    bool IsDeviceSuitable(VkPhysicalDevice device) const;
    int RateDeviceSuitability(VkPhysicalDevice device) const;

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height) const;

    void CreateSwapChainImageViews();

private:
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;
    QueueFamilyIndices m_QueueFamilyIndices;
    
    VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
    std::vector<VkImage> m_SwapChainImages;
    std::vector<VkImageView> m_SwapChainImageViews;
    VkFormat m_SwapChainImageFormat;
    VkExtent2D m_SwapChainExtent;
    
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    
    bool m_ValidationEnabled = false;
    GLFWwindow* m_Window = nullptr;

    const std::vector<const char*> m_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> m_DeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
    };
};

} // namespace V2D
