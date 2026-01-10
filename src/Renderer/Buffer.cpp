#include "Vulkan2D/Renderer/Buffer.h"
#include "Vulkan2D/Renderer/VulkanContext.h"
#include <stdexcept>
#include <cstring>

namespace V2D {

Buffer::Buffer(VulkanContext* context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    : m_Context(context), m_Size(size) {
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(context->GetDevice(), &bufferInfo, nullptr, &m_Buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context->GetDevice(), m_Buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = context->FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(context->GetDevice(), &allocInfo, nullptr, &m_Memory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory");
    }

    vkBindBufferMemory(context->GetDevice(), m_Buffer, m_Memory, 0);
}

Buffer::~Buffer() {
    if (m_MappedData) {
        Unmap();
    }
    if (m_Buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_Context->GetDevice(), m_Buffer, nullptr);
    }
    if (m_Memory != VK_NULL_HANDLE) {
        vkFreeMemory(m_Context->GetDevice(), m_Memory, nullptr);
    }
}

void Buffer::Map(VkDeviceSize size, VkDeviceSize offset) {
    vkMapMemory(m_Context->GetDevice(), m_Memory, offset, size, 0, &m_MappedData);
}

void Buffer::Unmap() {
    if (m_MappedData) {
        vkUnmapMemory(m_Context->GetDevice(), m_Memory);
        m_MappedData = nullptr;
    }
}

void Buffer::CopyData(const void* data, VkDeviceSize size) {
    if (!m_MappedData) {
        Map(size);
        memcpy(m_MappedData, data, size);
        Unmap();
    } else {
        memcpy(m_MappedData, data, size);
    }
}

void Buffer::Flush(VkDeviceSize size, VkDeviceSize offset) {
    VkMappedMemoryRange mappedRange{};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = m_Memory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    vkFlushMappedMemoryRanges(m_Context->GetDevice(), 1, &mappedRange);
}

void Buffer::CopyBuffer(VulkanContext* context, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = context->BeginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    context->EndSingleTimeCommands(commandBuffer);
}

// VertexBuffer implementation
VertexBuffer::VertexBuffer(VulkanContext* context, const void* data, VkDeviceSize size) {
    m_StagingBuffer = std::make_unique<Buffer>(
        context, size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    m_StagingBuffer->CopyData(data, size);

    m_Buffer = std::make_unique<Buffer>(
        context, size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    Buffer::CopyBuffer(context, m_StagingBuffer->GetBuffer(), m_Buffer->GetBuffer(), size);
    m_StagingBuffer.reset();
}

// IndexBuffer implementation
IndexBuffer::IndexBuffer(VulkanContext* context, const void* data, VkDeviceSize size, uint32_t indexCount)
    : m_IndexCount(indexCount) {
    
    m_StagingBuffer = std::make_unique<Buffer>(
        context, size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    m_StagingBuffer->CopyData(data, size);

    m_Buffer = std::make_unique<Buffer>(
        context, size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    Buffer::CopyBuffer(context, m_StagingBuffer->GetBuffer(), m_Buffer->GetBuffer(), size);
    m_StagingBuffer.reset();
}

// UniformBuffer implementation
UniformBuffer::UniformBuffer(VulkanContext* context, VkDeviceSize size) {
    m_Buffer = std::make_unique<Buffer>(
        context, size,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    m_Buffer->Map();
}

void UniformBuffer::Update(const void* data, VkDeviceSize size) {
    memcpy(m_Buffer->GetMappedData(), data, size);
}

} // namespace V2D
