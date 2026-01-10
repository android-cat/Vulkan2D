#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace V2D {

class VulkanContext;
class Shader;

struct PipelineConfig {
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    bool enableBlending = true;
    bool enableDepthTest = false;
    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_SRGB;
};

class Pipeline {
public:
    Pipeline(VulkanContext* context, Shader* shader, const PipelineConfig& config,
             const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
             const std::vector<VkPushConstantRange>& pushConstantRanges = {});
    ~Pipeline();

    // Non-copyable
    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    void Bind(VkCommandBuffer commandBuffer);
    
    VkPipeline GetPipeline() const { return m_Pipeline; }
    VkPipelineLayout GetLayout() const { return m_PipelineLayout; }

private:
    void CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
                              const std::vector<VkPushConstantRange>& pushConstantRanges);
    void CreatePipeline(Shader* shader, const PipelineConfig& config);

    VulkanContext* m_Context;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
};

} // namespace V2D
