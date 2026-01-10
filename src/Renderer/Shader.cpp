#include "Vulkan2D/Renderer/Shader.h"
#include "Vulkan2D/Renderer/VulkanContext.h"
#include <fstream>
#include <stdexcept>

namespace V2D {

Shader::Shader(VulkanContext* context, const std::string& vertexPath, const std::string& fragmentPath)
    : m_Context(context) {
    
    auto vertCode = ReadFile(vertexPath);
    auto fragCode = ReadFile(fragmentPath);

    m_VertexModule = CreateShaderModule(vertCode);
    m_FragmentModule = CreateShaderModule(fragCode);
}

Shader::~Shader() {
    if (m_VertexModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(m_Context->GetDevice(), m_VertexModule, nullptr);
    }
    if (m_FragmentModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(m_Context->GetDevice(), m_FragmentModule, nullptr);
    }
}

VkShaderModule Shader::CreateShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_Context->GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }

    return shaderModule;
}

std::vector<char> Shader::ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filename);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

std::vector<VkPipelineShaderStageCreateInfo> Shader::GetStageCreateInfos() const {
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = m_VertexModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = m_FragmentModule;
    fragShaderStageInfo.pName = "main";

    return {vertShaderStageInfo, fragShaderStageInfo};
}

} // namespace V2D
