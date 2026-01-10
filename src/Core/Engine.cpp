/**
 * @file Engine.cpp
 * @brief ゲームエンジンのメインクラス実装
 * 
 * Vulkan2Dエンジンの中核となるクラスで、
 * ウィンドウ、Vulkanコンテキスト、レンダラー、入力管理を統括する
 */

#include "Vulkan2D/Core/Engine.h"
#include <iostream>

namespace V2D {

/**
 * @brief エンジンのコンストラクタ
 * @param config エンジンの設定（ウィンドウサイズ、バリデーション有効化など）
 * 
 * 初期化順序:
 * 1. ウィンドウ作成
 * 2. Vulkanコンテキスト初期化
 * 3. 2Dレンダラー作成
 * 4. 入力マネージャー作成
 */
Engine::Engine(const EngineConfig& config) {
    // ウィンドウを作成
    m_Window = std::make_unique<Window>(config.windowConfig);
    
    // Vulkanコンテキストを作成・初期化
    m_VulkanContext = std::make_unique<VulkanContext>();
    m_VulkanContext->Initialize(m_Window->GetHandle(), config.enableValidation);
    
    // 2Dレンダラーを作成
    m_Renderer = std::make_unique<Renderer2D>(m_VulkanContext.get(), m_Window.get());
    
    // 入力マネージャーを作成
    m_Input = std::make_unique<InputManager>(m_Window.get());
    
    // オーディオマネージャーを作成
    m_Audio = std::make_unique<AudioManager>();
    
    // ウィンドウリサイズコールバックを設定
    m_Window->SetResizeCallback([this](int width, int height) {
        m_Renderer->OnWindowResize(width, height);
    });
    
    // フレーム時間計測の開始時刻を記録
    m_LastFrameTime = std::chrono::high_resolution_clock::now();
}

/**
 * @brief エンジンのデストラクタ
 * 
 * リソースの解放順序に注意が必要:
 * GPU処理の完了を待ってから、レンダラー→入力→Vulkan→ウィンドウの順で解放
 */
Engine::~Engine() {
    // GPUの処理が完了するまで待機
    if (m_VulkanContext) {
        m_VulkanContext->WaitIdle();
    }
    
    // 依存関係の逆順でリソースを解放
    m_Renderer.reset();
    m_Audio.reset();
    m_Input.reset();
    m_VulkanContext.reset();
    m_Window.reset();
}

/**
 * @brief メインゲームループを実行
 * @param onUpdate 毎フレーム呼ばれる更新コールバック（デルタタイムを受け取る）
 * @param onRender 毎フレーム呼ばれる描画コールバック
 * 
 * ゲームループの流れ:
 * 1. デルタタイム計算
 * 2. FPS計算
 * 3. イベントポーリング
 * 4. 入力更新
 * 5. ユーザー更新処理
 * 6. 描画開始
 * 7. ユーザー描画処理
 * 8. 描画終了・プレゼント
 */
void Engine::Run(UpdateCallback onUpdate, RenderCallback onRender) {
    m_Running = true;
    
    while (m_Running && !m_Window->ShouldClose()) {
        // デルタタイム（前フレームからの経過時間）を計算
        auto currentTime = std::chrono::high_resolution_clock::now();
        m_DeltaTime = std::chrono::duration<float>(currentTime - m_LastFrameTime).count();
        m_LastFrameTime = currentTime;
        m_TotalTime += m_DeltaTime;
        
        // FPS（フレームレート）を計算（1秒ごとに更新）
        static float fpsTimer = 0.0f;
        static int frameCount = 0;
        fpsTimer += m_DeltaTime;
        frameCount++;
        if (fpsTimer >= 1.0f) {
            m_FPS = static_cast<float>(frameCount) / fpsTimer;
            fpsTimer = 0.0f;
            frameCount = 0;
        }
        
        // ウィンドウイベントをポーリング
        m_Window->PollEvents();
        
        // 入力状態を更新
        m_Input->Update();
        
        // オーディオを更新（フェード処理など）
        m_Audio->Update(m_DeltaTime);
        
        // ユーザー定義の更新処理を呼び出し
        if (onUpdate) {
            onUpdate(m_DeltaTime);
        }
        
        // ウィンドウが最小化されている場合は描画をスキップ
        if (m_Window->IsMinimized()) {
            continue;
        }
        
        // フレーム描画を開始（スワップチェーンから画像を取得）
        if (!m_Renderer->BeginFrame()) {
            continue;
        }
        
        // ユーザー定義の描画処理を呼び出し
        if (onRender) {
            onRender();
        }
        
        // フレーム描画を終了（コマンドバッファ送信・プレゼント）
        m_Renderer->EndFrame();
    }
    
    // ループ終了後、GPUの処理完了を待機
    m_VulkanContext->WaitIdle();
}

/**
 * @brief エンジンを停止
 * 
 * 次のフレームでゲームループが終了する
 */
void Engine::Stop() {
    m_Running = false;
}

} // namespace V2D
