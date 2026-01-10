#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <memory>

namespace V2D {

class VulkanContext;

enum class TextureFilter {
    Nearest,    // Pixel-perfect, good for pixel art
    Linear      // Smooth interpolation
};

enum class TextureWrap {
    Repeat,
    ClampToEdge,
    MirroredRepeat
};

class Texture {
public:
    // Load from file (PNG, JPG, BMP, TGA, etc.)
    Texture(VulkanContext* context, const std::string& filepath);
    
    // Create from raw pixel data (RGBA format)
    Texture(VulkanContext* context, uint32_t width, uint32_t height, const void* pixels);
    
    // Create with custom settings
    Texture(VulkanContext* context, uint32_t width, uint32_t height, const void* pixels,
            TextureFilter filter, TextureWrap wrap);
    
    ~Texture();

    // Non-copyable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    VkImageView GetImageView() const { return m_ImageView; }
    VkSampler GetSampler() const { return m_Sampler; }
    VkDescriptorImageInfo GetDescriptorInfo() const;

    // Change filter/wrap modes (recreates sampler)
    void SetFilter(TextureFilter filter);
    void SetWrap(TextureWrap wrap);

    // Create a white 1x1 texture (useful for untextured sprites)
    static std::unique_ptr<Texture> CreateWhiteTexture(VulkanContext* context);
    
    // Create a colored texture
    static std::unique_ptr<Texture> CreateColorTexture(VulkanContext* context, uint32_t color);

private:
    void CreateImage(uint32_t width, uint32_t height, const void* pixels);
    void CreateImageView();
    void CreateSampler();
    void TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
    void CopyBufferToImage(VkBuffer buffer, uint32_t width, uint32_t height);

    VulkanContext* m_Context;
    uint32_t m_Width;
    uint32_t m_Height;
    VkImage m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VkSampler m_Sampler = VK_NULL_HANDLE;
    
    TextureFilter m_Filter = TextureFilter::Linear;
    TextureWrap m_Wrap = TextureWrap::Repeat;
};

} // namespace V2D
