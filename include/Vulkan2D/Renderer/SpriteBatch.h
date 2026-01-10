#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <unordered_map>

namespace V2D {

class VulkanContext;
class Texture;
class Buffer;
class Pipeline;
class Shader;
class Sprite;
class Camera2D;

struct Vertex2D {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texCoord;
};

class SpriteBatch {
public:
    SpriteBatch(VulkanContext* context, uint32_t maxSprites = 10000);
    ~SpriteBatch();

    // Non-copyable
    SpriteBatch(const SpriteBatch&) = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;

    void Begin(Camera2D* camera);
    void Draw(const Sprite& sprite);
    void Draw(Texture* texture, const glm::vec2& position, const glm::vec2& size, 
              const glm::vec4& color = glm::vec4(1.0f),
              float rotation = 0.0f, const glm::vec2& origin = glm::vec2(0.0f));
    void Draw(Texture* texture, const glm::vec2& position, const glm::vec2& size,
              const glm::vec2& uvMin, const glm::vec2& uvMax,
              const glm::vec4& color = glm::vec4(1.0f),
              float rotation = 0.0f, const glm::vec2& origin = glm::vec2(0.0f));
    void End();

    void Render(VkCommandBuffer commandBuffer);
    
    uint32_t GetSpriteCount() const { return m_SpriteCount; }
    uint32_t GetDrawCallCount() const { return m_DrawCallCount; }

    VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

private:
    struct BatchInfo {
        Texture* texture;
        uint32_t startIndex;
        uint32_t indexCount;
    };

    void CreateDescriptorSetLayout();
    void CreateDescriptorPool();
    VkDescriptorSet GetOrCreateDescriptorSet(Texture* texture);
    void Flush();

    VulkanContext* m_Context;
    
    std::unique_ptr<Shader> m_Shader;
    std::unique_ptr<Pipeline> m_Pipeline;
    
    std::vector<Vertex2D> m_Vertices;
    std::vector<uint32_t> m_Indices;
    std::unique_ptr<Buffer> m_VertexBuffer;
    std::unique_ptr<Buffer> m_IndexBuffer;
    
    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    std::unordered_map<Texture*, VkDescriptorSet> m_DescriptorSets;
    
    std::vector<BatchInfo> m_Batches;
    Texture* m_CurrentTexture = nullptr;
    Camera2D* m_CurrentCamera = nullptr;
    
    uint32_t m_MaxSprites;
    uint32_t m_SpriteCount = 0;
    uint32_t m_DrawCallCount = 0;
    bool m_IsBatching = false;

    std::unique_ptr<Texture> m_WhiteTexture;
};

} // namespace V2D
