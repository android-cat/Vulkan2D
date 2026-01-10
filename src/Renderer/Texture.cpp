#include "Vulkan2D/Renderer/Texture.h"
#include "Vulkan2D/Renderer/VulkanContext.h"
#include "Vulkan2D/Renderer/Buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdexcept>

namespace V2D {

Texture::Texture(VulkanContext* context, const std::string& filepath)
    : m_Context(context) {
    
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    
    if (!pixels) {
        throw std::runtime_error("Failed to load texture image: " + filepath);
    }

    m_Width = static_cast<uint32_t>(texWidth);
    m_Height = static_cast<uint32_t>(texHeight);

    CreateImage(m_Width, m_Height, pixels);
    stbi_image_free(pixels);
    
    CreateImageView();
    CreateSampler();
}

Texture::Texture(VulkanContext* context, uint32_t width, uint32_t height, const void* pixels)
    : m_Context(context), m_Width(width), m_Height(height) {
    
    CreateImage(width, height, pixels);
    CreateImageView();
    CreateSampler();
}

Texture::Texture(VulkanContext* context, uint32_t width, uint32_t height, const void* pixels,
                 TextureFilter filter, TextureWrap wrap)
    : m_Context(context), m_Width(width), m_Height(height), m_Filter(filter), m_Wrap(wrap) {
    
    CreateImage(width, height, pixels);
    CreateImageView();
    CreateSampler();
}

Texture::~Texture() {
    if (m_Sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_Context->GetDevice(), m_Sampler, nullptr);
    }
    if (m_ImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_Context->GetDevice(), m_ImageView, nullptr);
    }
    if (m_Image != VK_NULL_HANDLE) {
        vkDestroyImage(m_Context->GetDevice(), m_Image, nullptr);
    }
    if (m_ImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_Context->GetDevice(), m_ImageMemory, nullptr);
    }
}

void Texture::CreateImage(uint32_t width, uint32_t height, const void* pixels) {
    VkDeviceSize imageSize = width * height * 4;

    Buffer stagingBuffer(m_Context, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.CopyData(pixels, imageSize);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(m_Context->GetDevice(), &imageInfo, nullptr, &m_Image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_Context->GetDevice(), m_Image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = m_Context->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(m_Context->GetDevice(), &allocInfo, nullptr, &m_ImageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate image memory");
    }

    vkBindImageMemory(m_Context->GetDevice(), m_Image, m_ImageMemory, 0);

    TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage(stagingBuffer.GetBuffer(), width, height);
    TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Texture::CreateImageView() {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_Image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_Context->GetDevice(), &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture image view");
    }
}

void Texture::CreateSampler() {
    if (m_Sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_Context->GetDevice(), m_Sampler, nullptr);
        m_Sampler = VK_NULL_HANDLE;
    }

    VkFilter vkFilter = (m_Filter == TextureFilter::Nearest) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    
    VkSamplerAddressMode addressMode;
    switch (m_Wrap) {
        case TextureWrap::ClampToEdge:
            addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        case TextureWrap::MirroredRepeat:
            addressMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            break;
        default:
            addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = vkFilter;
    samplerInfo.minFilter = vkFilter;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable = (m_Filter == TextureFilter::Linear) ? VK_TRUE : VK_FALSE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = (m_Filter == TextureFilter::Nearest) ? 
                             VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(m_Context->GetDevice(), &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture sampler");
    }
}

void Texture::SetFilter(TextureFilter filter) {
    if (m_Filter != filter) {
        m_Filter = filter;
        CreateSampler();
    }
}

void Texture::SetWrap(TextureWrap wrap) {
    if (m_Wrap != wrap) {
        m_Wrap = wrap;
        CreateSampler();
    }
}

void Texture::TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = m_Context->BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_Image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::runtime_error("Unsupported layout transition");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);

    m_Context->EndSingleTimeCommands(commandBuffer);
}

void Texture::CopyBufferToImage(VkBuffer buffer, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = m_Context->BeginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    m_Context->EndSingleTimeCommands(commandBuffer);
}

VkDescriptorImageInfo Texture::GetDescriptorInfo() const {
    VkDescriptorImageInfo info{};
    info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    info.imageView = m_ImageView;
    info.sampler = m_Sampler;
    return info;
}

std::unique_ptr<Texture> Texture::CreateWhiteTexture(VulkanContext* context) {
    uint32_t whitePixel = 0xFFFFFFFF;
    return std::make_unique<Texture>(context, 1, 1, &whitePixel);
}

std::unique_ptr<Texture> Texture::CreateColorTexture(VulkanContext* context, uint32_t color) {
    return std::make_unique<Texture>(context, 1, 1, &color);
}

} // namespace V2D
