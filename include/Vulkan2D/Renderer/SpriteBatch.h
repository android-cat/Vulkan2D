#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <array>

namespace V2D {

class VulkanContext;
class Texture;
class Buffer;
class Pipeline;
class Shader;
class Sprite;
class Camera2D;

// レガシー頂点フォーマット（互換性のため保持）
struct Vertex2D {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texCoord;
};

// インスタンシング用クワッド頂点（4頂点で1つのクワッドを定義）
struct QuadVertex {
    glm::vec2 position;   // クワッドの相対位置 (-0.5 to 0.5)
    glm::vec2 texCoord;   // UV座標 (0 to 1)
};

// インスタンスデータ（各スプライト固有のデータ）
struct SpriteInstance {
    glm::vec2 position;   // ワールド座標位置
    glm::vec2 size;       // サイズ
    glm::vec2 origin;     // 回転原点
    float rotation;       // 回転角（ラジアン）
    float _padding;       // アライメント用パディング
    glm::vec4 color;      // 色
    glm::vec4 uvRect;     // UV矩形 (minU, minV, maxU, maxV)
};

// 描画統計情報
struct BatchStatistics {
    uint32_t spriteCount;      // スプライト数
    uint32_t drawCallCount;    // ドローコール数
    uint32_t instancedBatches; // インスタンシングで処理したバッチ数
    uint32_t textureBindCount; // テクスチャバインド回数
    float cpuTimeMs;           // CPU処理時間（ミリ秒）
};

class SpriteBatch {
public:
    // 最大同時処理スプライト数（デフォルト: 65536）
    static constexpr uint32_t DEFAULT_MAX_SPRITES = 65536;
    // インスタンシングの最小スプライト数閾値
    static constexpr uint32_t INSTANCING_THRESHOLD = 4;
    // フレーム内バッファリング数
    static constexpr uint32_t FRAME_BUFFER_COUNT = 2;

    SpriteBatch(VulkanContext* context, uint32_t maxSprites = DEFAULT_MAX_SPRITES);
    ~SpriteBatch();

    // Non-copyable
    SpriteBatch(const SpriteBatch&) = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;

    // === 描画API ===
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
    
    // === 統計情報 ===
    uint32_t GetSpriteCount() const { return m_Statistics.spriteCount; }
    uint32_t GetDrawCallCount() const { return m_Statistics.drawCallCount; }
    const BatchStatistics& GetStatistics() const { return m_Statistics; }
    
    // === 設定 ===
    void SetTextureSortingEnabled(bool enabled) { m_EnableTextureSorting = enabled; }
    bool IsTextureSortingEnabled() const { return m_EnableTextureSorting; }
    
    VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

private:
    // バッチ情報（テクスチャごとにグループ化）
    struct BatchInfo {
        Texture* texture;
        uint32_t startInstance;
        uint32_t instanceCount;
    };

    // ソート用スプライトデータ
    struct SpriteData {
        Texture* texture;
        SpriteInstance instance;
    };

    // フレームごとのバッファ
    struct FrameData {
        std::unique_ptr<Buffer> instanceStagingBuffer;  // ステージング（CPU側）
        std::unique_ptr<Buffer> instanceBuffer;         // Device Local（GPU側）
        bool needsUpload;
    };

    void CreateDescriptorSetLayout();
    void CreateDescriptorPool();
    void CreateQuadBuffers();
    void CreateInstanceBuffers();
    void CreateInstancedPipeline();
    VkDescriptorSet GetOrCreateDescriptorSet(Texture* texture);
    void UploadInstanceData();
    void SortBatches();
    void BuildBatches();

    VulkanContext* m_Context;
    
    // シェーダーとパイプライン
    std::unique_ptr<Shader> m_Shader;
    std::unique_ptr<Shader> m_InstancedShader;
    std::unique_ptr<Pipeline> m_Pipeline;
    std::unique_ptr<Pipeline> m_InstancedPipeline;
    
    // クワッドバッファ（全スプライト共通の4頂点）
    std::unique_ptr<Buffer> m_QuadVertexBuffer;
    std::unique_ptr<Buffer> m_QuadIndexBuffer;
    
    // レガシーバッファ（フォールバック用）
    std::vector<Vertex2D> m_Vertices;
    std::vector<uint32_t> m_Indices;
    std::unique_ptr<Buffer> m_VertexBuffer;
    std::unique_ptr<Buffer> m_IndexBuffer;
    
    // インスタンスバッファ（ダブルバッファリング）
    std::array<FrameData, FRAME_BUFFER_COUNT> m_FrameData;
    uint32_t m_CurrentFrameIndex = 0;
    
    // スプライトデータ
    std::vector<SpriteData> m_SpriteDataList;
    std::vector<SpriteInstance> m_Instances;
    std::vector<BatchInfo> m_Batches;
    
    // ディスクリプタ
    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    std::unordered_map<Texture*, VkDescriptorSet> m_DescriptorSets;
    
    // 状態
    Camera2D* m_CurrentCamera = nullptr;
    uint32_t m_MaxSprites;
    bool m_IsBatching = false;
    bool m_EnableTextureSorting = true;  // テクスチャソート有効化
    bool m_UseInstancing = true;         // インスタンシング有効化
    
    // 統計
    BatchStatistics m_Statistics{};

    std::unique_ptr<Texture> m_WhiteTexture;
};

} // namespace V2D
