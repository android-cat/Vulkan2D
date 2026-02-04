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
#include <chrono>

namespace V2D {

SpriteBatch::SpriteBatch(VulkanContext* context, uint32_t maxSprites)
    : m_Context(context), m_MaxSprites(maxSprites) {
    
    // 容量を事前確保（メモリ再アロケーションを防止）
    m_SpriteDataList.reserve(maxSprites);
    m_Instances.reserve(maxSprites);
    m_Batches.reserve(256);  // 通常十分な数
    
    // レガシーバッファ用
    m_Vertices.reserve(maxSprites * 4);
    m_Indices.reserve(maxSprites * 6);
    
    CreateDescriptorSetLayout();
    CreateDescriptorPool();
    
    // クワッドバッファを作成（インスタンシング用の4頂点）
    CreateQuadBuffers();
    
    // インスタンスバッファを作成（ダブルバッファリング）
    CreateInstanceBuffers();
    
    // 白テクスチャを作成（テクスチャなし描画用）
    m_WhiteTexture = Texture::CreateWhiteTexture(context);
    
    // レガシーシェーダー＆パイプライン
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
    
    // インスタンシングパイプラインを作成（将来用）
    CreateInstancedPipeline();
    
    // レガシーバッファ
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
}

SpriteBatch::~SpriteBatch() {
    m_Context->WaitIdle();
    
    if (m_DescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_Context->GetDevice(), m_DescriptorPool, nullptr);
    }
    if (m_DescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_Context->GetDevice(), m_DescriptorSetLayout, nullptr);
    }
}

void SpriteBatch::CreateQuadBuffers() {
    // クワッドの4頂点を定義（-0.5 ~ 0.5の正規化座標）
    std::array<QuadVertex, 4> quadVertices = {{
        {{-0.5f, -0.5f}, {0.0f, 0.0f}},  // 左上
        {{ 0.5f, -0.5f}, {1.0f, 0.0f}},  // 右上
        {{ 0.5f,  0.5f}, {1.0f, 1.0f}},  // 右下
        {{-0.5f,  0.5f}, {0.0f, 1.0f}}   // 左下
    }};
    
    std::array<uint16_t, 6> quadIndices = {0, 1, 2, 2, 3, 0};
    
    // クワッド頂点バッファ（Device Local + 転送）
    VkDeviceSize vertexSize = sizeof(QuadVertex) * quadVertices.size();
    
    // ステージングバッファ
    Buffer stagingVertex(m_Context, vertexSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingVertex.CopyData(quadVertices.data(), vertexSize);
    
    // Device Localバッファ（GPU専用メモリ - 高速アクセス）
    m_QuadVertexBuffer = std::make_unique<Buffer>(
        m_Context, vertexSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    Buffer::CopyBuffer(m_Context, stagingVertex.GetBuffer(), m_QuadVertexBuffer->GetBuffer(), vertexSize);
    
    // クワッドインデックスバッファ（Device Local）
    VkDeviceSize indexSize = sizeof(uint16_t) * quadIndices.size();
    
    Buffer stagingIndex(m_Context, indexSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingIndex.CopyData(quadIndices.data(), indexSize);
    
    m_QuadIndexBuffer = std::make_unique<Buffer>(
        m_Context, indexSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    Buffer::CopyBuffer(m_Context, stagingIndex.GetBuffer(), m_QuadIndexBuffer->GetBuffer(), indexSize);
}

void SpriteBatch::CreateInstanceBuffers() {
    VkDeviceSize instanceBufferSize = sizeof(SpriteInstance) * m_MaxSprites;
    
    for (uint32_t i = 0; i < FRAME_BUFFER_COUNT; i++) {
        // ステージングバッファ（CPU可視、永続マッピング）
        m_FrameData[i].instanceStagingBuffer = std::make_unique<Buffer>(
            m_Context, instanceBufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        m_FrameData[i].instanceStagingBuffer->Map();
        
        // Device Localバッファ（GPU専用、高速アクセス）
        m_FrameData[i].instanceBuffer = std::make_unique<Buffer>(
            m_Context, instanceBufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
        
        m_FrameData[i].needsUpload = false;
    }
}

void SpriteBatch::CreateInstancedPipeline() {
    // インスタンシングシェーダーをロード（存在する場合）
    try {
        m_InstancedShader = std::make_unique<Shader>(
            m_Context, 
            "shaders/sprite_instanced.vert.spv", 
            "shaders/sprite_instanced.frag.spv"
        );
    } catch (const std::exception& e) {
        // シェーダーが見つからない場合はインスタンシングを無効化
        m_UseInstancing = false;
        return;
    }
    
    // 将来のインスタンシングパイプライン実装用
    m_UseInstancing = false;
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
    poolSize.descriptorCount = 2048;  // より多くのテクスチャをサポート

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 2048;

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
    
    // データをクリア
    m_SpriteDataList.clear();
    m_Instances.clear();
    m_Batches.clear();
    m_Vertices.clear();
    m_Indices.clear();
    
    // 統計をリセット
    m_Statistics = BatchStatistics{};
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
    
    if (m_SpriteDataList.size() >= m_MaxSprites) {
        // 最大数に達した場合は無視
        return;
    }
    
    if (!texture) {
        texture = m_WhiteTexture.get();
    }
    
    // スプライトデータを追加（後でソート・バッチ化）
    SpriteData spriteData;
    spriteData.texture = texture;
    spriteData.instance.position = position;
    spriteData.instance.size = size;
    spriteData.instance.origin = origin;
    spriteData.instance.rotation = rotation;
    spriteData.instance._padding = 0.0f;
    spriteData.instance.color = color;
    spriteData.instance.uvRect = glm::vec4(uvMin.x, uvMin.y, uvMax.x, uvMax.y);
    
    m_SpriteDataList.push_back(spriteData);
}

// テクスチャでソートしてドローコールを削減
void SpriteBatch::SortBatches() {
    if (!m_EnableTextureSorting || m_SpriteDataList.empty()) {
        return;
    }
    
    // テクスチャポインタでソート（同じテクスチャを連続させる）
    std::stable_sort(m_SpriteDataList.begin(), m_SpriteDataList.end(),
        [](const SpriteData& a, const SpriteData& b) {
            return a.texture < b.texture;
        }
    );
}

// バッチを構築（同じテクスチャのスプライトをグループ化）
void SpriteBatch::BuildBatches() {
    if (m_SpriteDataList.empty()) {
        return;
    }
    
    Texture* currentTexture = nullptr;
    uint32_t batchStartIndex = 0;
    
    for (size_t i = 0; i < m_SpriteDataList.size(); i++) {
        const auto& spriteData = m_SpriteDataList[i];
        
        if (spriteData.texture != currentTexture) {
            // 前のバッチを記録
            if (currentTexture != nullptr && i > batchStartIndex) {
                BatchInfo batch;
                batch.texture = currentTexture;
                batch.startInstance = batchStartIndex;
                batch.instanceCount = static_cast<uint32_t>(i) - batchStartIndex;
                m_Batches.push_back(batch);
            }
            
            currentTexture = spriteData.texture;
            batchStartIndex = static_cast<uint32_t>(i);
        }
        
        m_Instances.push_back(spriteData.instance);
    }
    
    // 最後のバッチを記録
    if (currentTexture != nullptr) {
        BatchInfo batch;
        batch.texture = currentTexture;
        batch.startInstance = batchStartIndex;
        batch.instanceCount = static_cast<uint32_t>(m_SpriteDataList.size()) - batchStartIndex;
        m_Batches.push_back(batch);
    }
}

// インスタンスデータをGPUにアップロード
void SpriteBatch::UploadInstanceData() {
    if (m_Instances.empty()) {
        return;
    }
    
    auto& frameData = m_FrameData[m_CurrentFrameIndex];
    VkDeviceSize dataSize = m_Instances.size() * sizeof(SpriteInstance);
    
    memcpy(frameData.instanceStagingBuffer->GetMappedData(), 
           m_Instances.data(), 
           dataSize);
    
    frameData.needsUpload = true;
}

void SpriteBatch::End() {
    if (!m_IsBatching) {
        throw std::runtime_error("SpriteBatch::End called without Begin");
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // テクスチャでソート（ドローコール削減のため）
    SortBatches();
    
    // バッチを構築
    BuildBatches();
    
    // インスタンスデータをアップロード（将来のインスタンシング用）
    UploadInstanceData();
    
    // レガシー頂点データを生成
    for (const auto& spriteData : m_SpriteDataList) {
        const auto& inst = spriteData.instance;
        
        // 回転を適用した4頂点を計算
        std::array<glm::vec2, 4> corners = {
            glm::vec2(-inst.origin.x, -inst.origin.y),
            glm::vec2(inst.size.x - inst.origin.x, -inst.origin.y),
            glm::vec2(inst.size.x - inst.origin.x, inst.size.y - inst.origin.y),
            glm::vec2(-inst.origin.x, inst.size.y - inst.origin.y)
        };
        
        float cos_r = std::cos(inst.rotation);
        float sin_r = std::sin(inst.rotation);
        
        for (auto& corner : corners) {
            float x = corner.x * cos_r - corner.y * sin_r;
            float y = corner.x * sin_r + corner.y * cos_r;
            corner = glm::vec2(x, y) + inst.position;
        }
        
        uint32_t baseVertex = static_cast<uint32_t>(m_Vertices.size());
        
        glm::vec2 uvMin(inst.uvRect.x, inst.uvRect.y);
        glm::vec2 uvMax(inst.uvRect.z, inst.uvRect.w);
        
        m_Vertices.push_back({glm::vec3(corners[0], 0.0f), inst.color, glm::vec2(uvMin.x, uvMin.y)});
        m_Vertices.push_back({glm::vec3(corners[1], 0.0f), inst.color, glm::vec2(uvMax.x, uvMin.y)});
        m_Vertices.push_back({glm::vec3(corners[2], 0.0f), inst.color, glm::vec2(uvMax.x, uvMax.y)});
        m_Vertices.push_back({glm::vec3(corners[3], 0.0f), inst.color, glm::vec2(uvMin.x, uvMax.y)});
        
        m_Indices.push_back(baseVertex + 0);
        m_Indices.push_back(baseVertex + 1);
        m_Indices.push_back(baseVertex + 2);
        m_Indices.push_back(baseVertex + 2);
        m_Indices.push_back(baseVertex + 3);
        m_Indices.push_back(baseVertex + 0);
    }
    
    // バッファにアップロード
    if (!m_Vertices.empty()) {
        memcpy(m_VertexBuffer->GetMappedData(), m_Vertices.data(), 
               m_Vertices.size() * sizeof(Vertex2D));
    }
    if (!m_Indices.empty()) {
        memcpy(m_IndexBuffer->GetMappedData(), m_Indices.data(),
               m_Indices.size() * sizeof(uint32_t));
    }
    
    // 統計を更新
    m_Statistics.spriteCount = static_cast<uint32_t>(m_SpriteDataList.size());
    m_Statistics.drawCallCount = static_cast<uint32_t>(m_Batches.size());
    m_Statistics.instancedBatches = 0;
    m_Statistics.textureBindCount = static_cast<uint32_t>(m_Batches.size());
    
    auto endTime = std::chrono::high_resolution_clock::now();
    m_Statistics.cpuTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    m_IsBatching = false;
    
    // 次フレーム用にバッファインデックスを切り替え
    m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % FRAME_BUFFER_COUNT;
}

void SpriteBatch::Render(VkCommandBuffer commandBuffer) {
    if (m_Batches.empty()) return;
    
    m_Pipeline->Bind(commandBuffer);
    
    // ビューポートとシザーを設定
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
    
    // ビュー投影行列をプッシュ
    glm::mat4 viewProj = m_CurrentCamera->GetViewProjectionMatrix();
    vkCmdPushConstants(commandBuffer, m_Pipeline->GetLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &viewProj);
    
    // 頂点バッファとインデックスバッファをバインド
    VkBuffer vertexBuffers[] = {m_VertexBuffer->GetBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    
    // 各バッチを描画
    uint32_t indexOffset = 0;
    for (const auto& batch : m_Batches) {
        VkDescriptorSet descriptorSet = GetOrCreateDescriptorSet(batch.texture);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_Pipeline->GetLayout(), 0, 1, &descriptorSet, 0, nullptr);
        
        uint32_t indexCount = batch.instanceCount * 6;
        vkCmdDrawIndexed(commandBuffer, indexCount, 1, indexOffset, 0, 0);
        indexOffset += indexCount;
    }
}

} // namespace V2D
