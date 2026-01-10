#include "Vulkan2D/Renderer/SpriteBatch.h"
#include "Vulkan2D/Renderer/VulkanContext.h"
#include "Vulkan2D/Renderer/Buffer.h"
#include "Vulkan2D/Renderer/Pipeline.h"
#include "Vulkan2D/Renderer/Shader.h"
#include "Vulkan2D/Renderer/Texture.h"
#include "Vulkan2D/Graphics/Sprite.h"
#include "Vulkan2D/Graphics/Camera2D.h"
#include <stdexcept>
#include <cstring>
#include <array>

namespace V2D {

SpriteBatch::SpriteBatch(VulkanContext* context, uint32_t maxSprites)
    : m_Context(context), m_MaxSprites(maxSprites) {
    
    // Reserve space for vertices and indices
    m_Vertices.reserve(maxSprites * 4);
    m_Indices.reserve(maxSprites * 6);
    
    // Create vertex and index buffers
    VkDeviceSize vertexBufferSize = sizeof(Vertex2D) * maxSprites * 4;
    VkDeviceSize indexBufferSize = sizeof(uint32_t) * maxSprites * 6;
    
    m_VertexBuffer = std::make_unique<Buffer>(
        context, vertexBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    m_VertexBuffer->Map();
    
    m_IndexBuffer = std::make_unique<Buffer>(
        context, indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    m_IndexBuffer->Map();
    
    CreateDescriptorSetLayout();
    CreateDescriptorPool();
    
    // Create white texture for untextured drawing
    m_WhiteTexture = Texture::CreateWhiteTexture(context);
    
    // Create shader and pipeline
    m_Shader = std::make_unique<Shader>(context, "shaders/sprite.vert.spv", "shaders/sprite.frag.spv");
    
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);
    
    PipelineConfig config{};
    config.colorFormat = context->GetSwapChainImageFormat();
    
    m_Pipeline = std::make_unique<Pipeline>(
        context, m_Shader.get(), config,
        std::vector<VkDescriptorSetLayout>{m_DescriptorSetLayout},
        std::vector<VkPushConstantRange>{pushConstantRange}
    );
}

SpriteBatch::~SpriteBatch() {
    if (m_DescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_Context->GetDevice(), m_DescriptorPool, nullptr);
    }
    if (m_DescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_Context->GetDevice(), m_DescriptorSetLayout, nullptr);
    }
}

void SpriteBatch::CreateDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;

    if (vkCreateDescriptorSetLayout(m_Context->GetDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
}

void SpriteBatch::CreateDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1000; // Support up to 1000 unique textures

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1000;

    if (vkCreateDescriptorPool(m_Context->GetDevice(), &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }
}

VkDescriptorSet SpriteBatch::GetOrCreateDescriptorSet(Texture* texture) {
    auto it = m_DescriptorSets.find(texture);
    if (it != m_DescriptorSets.end()) {
        return it->second;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_DescriptorSetLayout;

    VkDescriptorSet descriptorSet;
    if (vkAllocateDescriptorSets(m_Context->GetDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set");
    }

    VkDescriptorImageInfo imageInfo = texture->GetDescriptorInfo();

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &descriptorWrite, 0, nullptr);

    m_DescriptorSets[texture] = descriptorSet;
    return descriptorSet;
}

void SpriteBatch::Begin(Camera2D* camera) {
    if (m_IsBatching) {
        throw std::runtime_error("SpriteBatch::Begin called without ending previous batch");
    }
    
    m_IsBatching = true;
    m_CurrentCamera = camera;
    m_Vertices.clear();
    m_Indices.clear();
    m_Batches.clear();
    m_SpriteCount = 0;
    m_DrawCallCount = 0;
    m_CurrentTexture = nullptr;
}

void SpriteBatch::Draw(const Sprite& sprite) {
    Texture* texture = sprite.GetTexture().get();
    if (!texture) {
        texture = m_WhiteTexture.get();
    }
    
    Draw(texture, sprite.transform.position, sprite.GetSize(),
         sprite.GetUVMin(), sprite.GetUVMax(),
         sprite.GetColor(), sprite.transform.rotation, sprite.transform.origin);
}

void SpriteBatch::Draw(Texture* texture, const glm::vec2& position, const glm::vec2& size,
                       const glm::vec4& color, float rotation, const glm::vec2& origin) {
    Draw(texture, position, size, glm::vec2(0.0f), glm::vec2(1.0f), color, rotation, origin);
}

void SpriteBatch::Draw(Texture* texture, const glm::vec2& position, const glm::vec2& size,
                       const glm::vec2& uvMin, const glm::vec2& uvMax,
                       const glm::vec4& color, float rotation, const glm::vec2& origin) {
    if (!m_IsBatching) {
        throw std::runtime_error("SpriteBatch::Draw called without Begin");
    }
    
    if (m_SpriteCount >= m_MaxSprites) {
        Flush();
    }
    
    if (!texture) {
        texture = m_WhiteTexture.get();
    }
    
    // Check if we need to start a new batch (texture change)
    if (texture != m_CurrentTexture) {
        if (m_CurrentTexture != nullptr) {
            // Record current batch
            BatchInfo batch;
            batch.texture = m_CurrentTexture;
            batch.startIndex = m_Batches.empty() ? 0 : 
                               m_Batches.back().startIndex + m_Batches.back().indexCount;
            batch.indexCount = static_cast<uint32_t>(m_Indices.size()) - batch.startIndex;
            if (batch.indexCount > 0) {
                m_Batches.push_back(batch);
            }
        }
        m_CurrentTexture = texture;
    }
    
    // Calculate vertices with rotation
    glm::vec2 halfSize = size * 0.5f;
    
    // Corners relative to origin
    std::array<glm::vec2, 4> corners = {
        glm::vec2(-origin.x, -origin.y),
        glm::vec2(size.x - origin.x, -origin.y),
        glm::vec2(size.x - origin.x, size.y - origin.y),
        glm::vec2(-origin.x, size.y - origin.y)
    };
    
    // Apply rotation
    float cos_r = std::cos(rotation);
    float sin_r = std::sin(rotation);
    
    for (auto& corner : corners) {
        float x = corner.x * cos_r - corner.y * sin_r;
        float y = corner.x * sin_r + corner.y * cos_r;
        corner = glm::vec2(x, y) + position;
    }
    
    uint32_t baseVertex = static_cast<uint32_t>(m_Vertices.size());
    
    // Add vertices
    m_Vertices.push_back({glm::vec3(corners[0], 0.0f), color, glm::vec2(uvMin.x, uvMin.y)});
    m_Vertices.push_back({glm::vec3(corners[1], 0.0f), color, glm::vec2(uvMax.x, uvMin.y)});
    m_Vertices.push_back({glm::vec3(corners[2], 0.0f), color, glm::vec2(uvMax.x, uvMax.y)});
    m_Vertices.push_back({glm::vec3(corners[3], 0.0f), color, glm::vec2(uvMin.x, uvMax.y)});
    
    // Add indices
    m_Indices.push_back(baseVertex + 0);
    m_Indices.push_back(baseVertex + 1);
    m_Indices.push_back(baseVertex + 2);
    m_Indices.push_back(baseVertex + 2);
    m_Indices.push_back(baseVertex + 3);
    m_Indices.push_back(baseVertex + 0);
    
    m_SpriteCount++;
}

void SpriteBatch::End() {
    if (!m_IsBatching) {
        throw std::runtime_error("SpriteBatch::End called without Begin");
    }
    
    // Record final batch
    if (m_CurrentTexture != nullptr && !m_Vertices.empty()) {
        BatchInfo batch;
        batch.texture = m_CurrentTexture;
        batch.startIndex = m_Batches.empty() ? 0 : 
                           m_Batches.back().startIndex + m_Batches.back().indexCount;
        batch.indexCount = static_cast<uint32_t>(m_Indices.size()) - batch.startIndex;
        if (batch.indexCount > 0) {
            m_Batches.push_back(batch);
        }
    }
    
    // Upload vertex and index data
    if (!m_Vertices.empty()) {
        memcpy(m_VertexBuffer->GetMappedData(), m_Vertices.data(), 
               m_Vertices.size() * sizeof(Vertex2D));
    }
    if (!m_Indices.empty()) {
        memcpy(m_IndexBuffer->GetMappedData(), m_Indices.data(),
               m_Indices.size() * sizeof(uint32_t));
    }
    
    m_DrawCallCount = static_cast<uint32_t>(m_Batches.size());
    m_IsBatching = false;
}

void SpriteBatch::Flush() {
    // This would be called during drawing if we exceed max sprites
    // For now, we just prevent overflow
}

void SpriteBatch::Render(VkCommandBuffer commandBuffer) {
    if (m_Batches.empty()) return;
    
    m_Pipeline->Bind(commandBuffer);
    
    // Set viewport and scissor
    VkExtent2D extent = m_Context->GetSwapChainExtent();
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    
    // Push view-projection matrix
    glm::mat4 viewProj = m_CurrentCamera->GetViewProjectionMatrix();
    vkCmdPushConstants(commandBuffer, m_Pipeline->GetLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &viewProj);
    
    // Bind vertex and index buffers
    VkBuffer vertexBuffers[] = {m_VertexBuffer->GetBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    
    // Draw each batch
    for (const auto& batch : m_Batches) {
        VkDescriptorSet descriptorSet = GetOrCreateDescriptorSet(batch.texture);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_Pipeline->GetLayout(), 0, 1, &descriptorSet, 0, nullptr);
        
        vkCmdDrawIndexed(commandBuffer, batch.indexCount, 1, batch.startIndex, 0, 0);
    }
}

} // namespace V2D
