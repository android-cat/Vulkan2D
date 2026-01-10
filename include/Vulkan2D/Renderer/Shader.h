#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>

namespace V2D {

class VulkanContext;

class Shader {
public:
    Shader(VulkanContext* context, const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    // Non-copyable
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    VkShaderModule GetVertexModule() const { return m_VertexModule; }
    VkShaderModule GetFragmentModule() const { return m_FragmentModule; }

    std::vector<VkPipelineShaderStageCreateInfo> GetStageCreateInfos() const;

private:
    VkShaderModule CreateShaderModule(const std::vector<char>& code);
    static std::vector<char> ReadFile(const std::string& filename);

    VulkanContext* m_Context;
    VkShaderModule m_VertexModule = VK_NULL_HANDLE;
    VkShaderModule m_FragmentModule = VK_NULL_HANDLE;
};

} // namespace V2D
