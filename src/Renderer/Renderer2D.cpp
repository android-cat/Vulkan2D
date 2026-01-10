#include "Vulkan2D/Renderer/Renderer2D.h"
#include "Vulkan2D/Renderer/VulkanContext.h"
#include "Vulkan2D/Renderer/SpriteBatch.h"
#include "Vulkan2D/Core/Window.h"
#include <stdexcept>
#include <array>

namespace V2D {

Renderer2D::Renderer2D(VulkanContext* context, Window* window)
    : m_Context(context), m_Window(window) {
    
    m_Context->CreateSwapChain(window->GetWidth(), window->GetHeight());
    CreateCommandBuffers();
    CreateSyncObjects();
    m_SpriteBatch = std::make_unique<SpriteBatch>(context);
}

Renderer2D::~Renderer2D() {
    m_Context->WaitIdle();
    
    m_SpriteBatch.reset();
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(m_Context->GetDevice(), m_ImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_Context->GetDevice(), m_RenderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_Context->GetDevice(), m_InFlightFences[i], nullptr);
    }
}

void Renderer2D::CreateCommandBuffers() {
    m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_Context->GetCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

    if (vkAllocateCommandBuffers(m_Context->GetDevice(), &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }
}

void Renderer2D::CreateSyncObjects() {
    m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_ImagesInFlight.resize(m_Context->GetImageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_Context->GetDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_Context->GetDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_Context->GetDevice(), &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects");
        }
    }
}

bool Renderer2D::BeginFrame() {
    // Wait for the frame to be finished
    vkWaitForFences(m_Context->GetDevice(), 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(m_Context->GetDevice(), m_Context->GetSwapChain(),
                                            UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame],
                                            VK_NULL_HANDLE, &m_CurrentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain();
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image");
    }

    // Only reset fence if we are submitting work
    vkResetFences(m_Context->GetDevice(), 1, &m_InFlightFences[m_CurrentFrame]);

    VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer");
    }

    BeginRendering(commandBuffer);
    return true;
}

void Renderer2D::EndFrame() {
    VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
    
    EndRendering(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_Context->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {m_Context->GetSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_CurrentImageIndex;

    VkResult result = vkQueuePresentKHR(m_Context->GetPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized) {
        m_FramebufferResized = false;
        RecreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image");
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer2D::BeginRendering(VkCommandBuffer commandBuffer) {
    // Get the actual swap chain image
    VkImage swapChainImage = m_Context->GetSwapChainImages()[m_CurrentImageIndex];
    
    // Transition image to color attachment optimal using synchronization2
    VkImageMemoryBarrier2 imageBarrierBegin{};
    imageBarrierBegin.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imageBarrierBegin.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    imageBarrierBegin.srcAccessMask = 0;
    imageBarrierBegin.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    imageBarrierBegin.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    imageBarrierBegin.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageBarrierBegin.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageBarrierBegin.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrierBegin.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrierBegin.image = swapChainImage;
    imageBarrierBegin.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrierBegin.subresourceRange.baseMipLevel = 0;
    imageBarrierBegin.subresourceRange.levelCount = 1;
    imageBarrierBegin.subresourceRange.baseArrayLayer = 0;
    imageBarrierBegin.subresourceRange.layerCount = 1;

    VkDependencyInfo dependencyInfoBegin{};
    dependencyInfoBegin.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfoBegin.imageMemoryBarrierCount = 1;
    dependencyInfoBegin.pImageMemoryBarriers = &imageBarrierBegin;
    
    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfoBegin);
    
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = m_Context->GetSwapChainImageViews()[m_CurrentImageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = m_ClearColor;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = m_Context->GetSwapChainExtent();
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
}

void Renderer2D::EndRendering(VkCommandBuffer commandBuffer) {
    vkCmdEndRendering(commandBuffer);

    // Get the actual swap chain image
    VkImage swapChainImage = m_Context->GetSwapChainImages()[m_CurrentImageIndex];

    // Transition to present using synchronization2
    VkImageMemoryBarrier2 imageBarrier{};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    imageBarrier.dstAccessMask = 0;
    imageBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.image = swapChainImage;
    imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = 1;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &imageBarrier;
    
    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}

void Renderer2D::SetClearColor(float r, float g, float b, float a) {
    m_ClearColor = {{r, g, b, a}};
}

void Renderer2D::OnWindowResize(int width, int height) {
    m_FramebufferResized = true;
}

void Renderer2D::RecreateSwapChain() {
    int width = m_Window->GetWidth();
    int height = m_Window->GetHeight();
    
    while (width == 0 || height == 0) {
        m_Window->PollEvents();
        width = m_Window->GetWidth();
        height = m_Window->GetHeight();
    }
    
    m_Context->WaitIdle();
    m_Context->RecreateSwapChain(width, height);
    
    // Reset image fences for new swap chain
    m_ImagesInFlight.resize(m_Context->GetImageCount(), VK_NULL_HANDLE);
}

} // namespace V2D
