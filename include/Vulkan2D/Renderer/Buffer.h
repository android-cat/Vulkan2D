#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace V2D {

class VulkanContext;

class Buffer {
public:
    Buffer(VulkanContext* context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    ~Buffer();

    // Non-copyable
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    void Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void Unmap();
    void CopyData(const void* data, VkDeviceSize size);
    void Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

    VkBuffer GetBuffer() const { return m_Buffer; }
    VkDeviceMemory GetMemory() const { return m_Memory; }
    VkDeviceSize GetSize() const { return m_Size; }
    void* GetMappedData() const { return m_MappedData; }

    static void CopyBuffer(VulkanContext* context, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

private:
    VulkanContext* m_Context;
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    VkDeviceSize m_Size;
    void* m_MappedData = nullptr;
};

// Specialized buffer types
class VertexBuffer {
public:
    VertexBuffer(VulkanContext* context, const void* data, VkDeviceSize size);
    
    VkBuffer GetBuffer() const { return m_Buffer->GetBuffer(); }

private:
    std::unique_ptr<Buffer> m_StagingBuffer;
    std::unique_ptr<Buffer> m_Buffer;
};

class IndexBuffer {
public:
    IndexBuffer(VulkanContext* context, const void* data, VkDeviceSize size, uint32_t indexCount);
    
    VkBuffer GetBuffer() const { return m_Buffer->GetBuffer(); }
    uint32_t GetIndexCount() const { return m_IndexCount; }

private:
    std::unique_ptr<Buffer> m_StagingBuffer;
    std::unique_ptr<Buffer> m_Buffer;
    uint32_t m_IndexCount;
};

class UniformBuffer {
public:
    UniformBuffer(VulkanContext* context, VkDeviceSize size);
    
    void Update(const void* data, VkDeviceSize size);
    VkBuffer GetBuffer() const { return m_Buffer->GetBuffer(); }

private:
    std::unique_ptr<Buffer> m_Buffer;
};

} // namespace V2D
