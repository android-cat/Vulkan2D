/**
 * @file main.cpp
 * @brief Vulkan2Dエンジンのサンプルアプリケーション
 * 
 * このサンプルでは以下の機能をデモンストレーションする:
 * - スプライトの描画とアニメーション
 * - カメラの移動とズーム
 * - テキスト描画（FPS表示、操作説明）
 * - 入力処理（キーボード、マウスホイール）
 * - サウンド再生（BGM、効果音）
 */

#include <Vulkan2D/Vulkan2D.h>
#include <iostream>
#include <cmath>

int main() {
    try {
        // === エンジン設定 ===
        V2D::EngineConfig config;
        config.windowConfig.title = "Vulkan2D Example";  // ウィンドウタイトル
        config.windowConfig.width = 1280;                // ウィンドウ幅
        config.windowConfig.height = 720;                // ウィンドウ高さ
        config.enableValidation = true;                  // Vulkanバリデーション有効

        // エンジンを作成
        V2D::Engine engine(config);

        // === カメラ設定 ===
        // 左上が原点(0,0)、右がX+、下がY+の座標系
        V2D::Camera2D camera(
            static_cast<float>(config.windowConfig.width),
            static_cast<float>(config.windowConfig.height)
        );

        // === テキスト描画の準備 ===
        V2D::TextRenderer textRenderer(engine.GetVulkanContext());

        // フォントを読み込み（日本語フォントを優先）
        std::unique_ptr<V2D::Font> font;
        try {
            // メイリオフォント（日本語対応）を試す
            font = std::make_unique<V2D::Font>(
                engine.GetVulkanContext(),
                "C:/Windows/Fonts/meiryo.ttc",
                32  // フォントサイズ（ピクセル）
            );
        } catch (...) {
            try {
                // フォールバック: Arialフォント
                font = std::make_unique<V2D::Font>(
                    engine.GetVulkanContext(),
                    "C:/Windows/Fonts/arial.ttf",
                    32
                );
            } catch (const std::exception& e) {
                std::cerr << "警告: フォント読み込み失敗: " << e.what() << std::endl;
            }
        }

        // === テスト用テクスチャ読み込み ===
        std::shared_ptr<V2D::Texture> sampleTexture;
        bool hasSampleTexture = false;
        try {
            // sample.pngを読み込み
            sampleTexture = std::make_shared<V2D::Texture>(
                engine.GetVulkanContext(),
                "sample.png"
            );
            hasSampleTexture = true;
            std::cout << "成功: sample.png を読み込みました" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "注意: sample.png の読み込みをスキップ: " << e.what() << std::endl;
        }

        // === スプライト作成 ===
        std::vector<V2D::Sprite> sprites;
        
        // sample.pngのスプライト（読み込めた場合のみ）
        if (hasSampleTexture) {
            V2D::Sprite sampleSprite;
            sampleSprite.SetTexture(sampleTexture);
            sampleSprite.SetSize(glm::vec2(200.0f, 200.0f));  // 200x200のサイズで表示
            sampleSprite.transform.position = glm::vec2(1000.0f, 100.0f);  // 右上に配置
            sampleSprite.transform.origin = glm::vec2(100.0f, 100.0f);
            sprites.push_back(sampleSprite);
        }
        
        // 虹色の四角形を10個作成
        for (int i = 0; i < 10; i++) {
            V2D::Sprite sprite;
            sprite.SetSize(glm::vec2(100.0f, 100.0f));  // サイズ 100x100
            sprite.transform.position = glm::vec2(
                50.0f + i * 110.0f,                     // X: 左から右へ配置
                300.0f + std::sin(i * 0.5f) * 80.0f     // Y: 波形に配置
            );
            sprite.transform.origin = glm::vec2(50.0f, 50.0f);  // 中心を回転の原点に
            
            // HSVから虹色を計算
            float hue = static_cast<float>(i) / 10.0f;
            float r = std::abs(std::sin(hue * 3.14159f * 2.0f));
            float g = std::abs(std::sin((hue + 0.333f) * 3.14159f * 2.0f));
            float b = std::abs(std::sin((hue + 0.666f) * 3.14159f * 2.0f));
            sprite.SetColor(glm::vec4(r, g, b, 1.0f));
            
            sprites.push_back(sprite);
        }

        float time = 0.0f;           // 経過時間（アニメーション用）
        float cameraSpeed = 300.0f;  // カメラ移動速度（ピクセル/秒）

        // === サウンドの準備（オプション） ===
        // サウンドファイルがある場合のみ再生
        std::unique_ptr<V2D::Sound> bgm;
        std::unique_ptr<V2D::Sound> sfx;
        V2D::AudioManager* audio = engine.GetAudio();
        
        // BGMをロード（例: assets/bgm.wav がある場合）
        try {
            // bgm = std::make_unique<V2D::Sound>("assets/bgm.wav", true);  // ストリーミングモード
            // audio->PlayBGM(bgm.get(), 0.5f, 1.0f);  // 音量50%、1秒フェードイン
        } catch (const std::exception& e) {
            std::cerr << "注意: BGMのロードをスキップ: " << e.what() << std::endl;
        }
        
        // 効果音をロード（例: assets/click.wav がある場合）
        try {
            // sfx = std::make_unique<V2D::Sound>("assets/click.wav");  // メモリロード
        } catch (const std::exception& e) {
            std::cerr << "注意: 効果音のロードをスキップ: " << e.what() << std::endl;
        }

        // === メインゲームループ ===
        engine.Run(
            // 更新コールバック（毎フレーム呼ばれる）
            [&](float deltaTime) {
                time += deltaTime;
                
                // 入力マネージャーを取得
                V2D::InputManager* input = engine.GetInput();
                
                // === カメラ移動（WASD） ===
                // W:上, S:下, A:左, D:右（Y+は下方向）
                glm::vec2 cameraMove(0.0f);
                if (input->IsKeyDown(V2D::Key::W)) cameraMove.y -= 1.0f;  // 上
                if (input->IsKeyDown(V2D::Key::S)) cameraMove.y += 1.0f;  // 下
                if (input->IsKeyDown(V2D::Key::A)) cameraMove.x -= 1.0f;  // 左
                if (input->IsKeyDown(V2D::Key::D)) cameraMove.x += 1.0f;  // 右
                
                if (glm::length(cameraMove) > 0.0f) {
                    // 斜め移動でも速度が一定になるよう正規化
                    camera.Move(glm::normalize(cameraMove) * cameraSpeed * deltaTime);
                }
                
                // === カメラズーム（マウスホイール） ===
                float scroll = input->GetScrollDelta();
                if (scroll != 0.0f) {
                    camera.Zoom(scroll * 0.1f);
                }
                
                // === ESCキーで終了 ===
                if (input->IsKeyPressed(V2D::Key::Escape)) {
                    engine.Stop();
                }
                
                // === Spaceキーで効果音再生 ===
                if (input->IsKeyPressed(V2D::Key::Space) && sfx) {
                    audio->Play(sfx.get(), 0.8f);  // 音量80%で再生
                }
                
                // === スプライトアニメーション ===
                for (size_t i = 0; i < sprites.size(); i++) {
                    // 回転アニメーション
                    sprites[i].transform.rotation = time + i * 0.3f;
                    // 上下の波形アニメーション
                    sprites[i].transform.position.y = 300.0f + std::sin(time * 2.0f + i * 0.5f) * 80.0f;
                }
                
                // ウィンドウリサイズに対応
                camera.SetViewport(
                    static_cast<float>(engine.GetWindow()->GetWidth()),
                    static_cast<float>(engine.GetWindow()->GetHeight())
                );
            },

            // 描画コールバック（毎フレーム呼ばれる）
            [&]() {
                V2D::Renderer2D* renderer = engine.GetRenderer();
                V2D::SpriteBatch* batch = renderer->GetSpriteBatch();
                
                // スプライトバッチ開始
                batch->Begin(&camera);
                
                // === 全スプライトを描画 ===
                for (const auto& sprite : sprites) {
                    batch->Draw(sprite);
                }
                
                // === 追加の矩形を直接描画 ===
                // テクスチャなし（nullptr）で黄色の半透明矩形
                batch->Draw(nullptr, glm::vec2(100.0f, 150.0f), glm::vec2(200.0f, 50.0f),
                           glm::vec4(1.0f, 1.0f, 0.0f, 0.8f));

                // === テキスト描画 ===
                if (font) {
                    // FPS表示（左上）
                    std::string fpsText = "FPS: " + std::to_string(static_cast<int>(engine.GetFPS()));
                    textRenderer.DrawText(batch, font.get(), fpsText,
                                         glm::vec2(10.0f, 10.0f),
                                         glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

                    // 操作説明
                    textRenderer.DrawText(batch, font.get(), "WASD: Move Camera",
                                         glm::vec2(10.0f, 50.0f),
                                         glm::vec4(0.8f, 0.8f, 0.8f, 1.0f), 0.7f);
                    
                    textRenderer.DrawText(batch, font.get(), "Mouse Wheel: Zoom",
                                         glm::vec2(10.0f, 80.0f),
                                         glm::vec4(0.8f, 0.8f, 0.8f, 1.0f), 0.7f);

                    textRenderer.DrawText(batch, font.get(), "ESC: Exit",
                                         glm::vec2(10.0f, 110.0f),
                                         glm::vec4(0.8f, 0.8f, 0.8f, 1.0f), 0.7f);
                    
                    textRenderer.DrawText(batch, font.get(), "Space: Play SFX",
                                         glm::vec2(10.0f, 140.0f),
                                         glm::vec4(0.8f, 0.8f, 0.8f, 1.0f), 0.7f);

                    // タイトル（影付き、中央揃え）
                    textRenderer.DrawTextWithShadow(batch, font.get(), "Vulkan2D Engine",
                                                   glm::vec2(640.0f, 50.0f),
                                                   glm::vec4(1.0f, 0.9f, 0.2f, 1.0f),   // 金色
                                                   glm::vec4(0.0f, 0.0f, 0.0f, 0.5f),   // 半透明黒影
                                                   glm::vec2(3.0f, 3.0f),               // 影オフセット
                                                   1.5f, V2D::TextAlign::Center);

                    // サブタイトル
                    textRenderer.DrawText(batch, font.get(), "Hello World!",
                                         glm::vec2(640.0f, 100.0f),
                                         glm::vec4(0.5f, 1.0f, 0.5f, 1.0f),
                                         1.0f, V2D::TextAlign::Center);

                    // === 日本語テキストテスト ===
                    // ひらがなテスト
                    textRenderer.DrawText(batch, font.get(), "ひらがな: こんにちは世界",
                                         glm::vec2(640.0f, 450.0f),
                                         glm::vec4(1.0f, 0.5f, 0.8f, 1.0f),
                                         1.0f, V2D::TextAlign::Center);
                    
                    // カタカナテスト
                    textRenderer.DrawText(batch, font.get(), "カタカナ: ヴァルカン エンジン",
                                         glm::vec2(640.0f, 490.0f),
                                         glm::vec4(0.5f, 0.8f, 1.0f, 1.0f),
                                         1.0f, V2D::TextAlign::Center);
                    
                    // 漢字テスト
                    textRenderer.DrawText(batch, font.get(), "漢字: 日本語描画成功",
                                         glm::vec2(640.0f, 530.0f),
                                         glm::vec4(1.0f, 1.0f, 0.5f, 1.0f),
                                         1.0f, V2D::TextAlign::Center);
                    
                    // 混合テスト（影付き）
                    textRenderer.DrawTextWithShadow(batch, font.get(), 
                                                   "混合テスト: Vulkan2Dで日本語OK!",
                                                   glm::vec2(640.0f, 580.0f),
                                                   glm::vec4(0.2f, 1.0f, 0.5f, 1.0f),
                                                   glm::vec4(0.0f, 0.0f, 0.0f, 0.7f),
                                                   glm::vec2(2.0f, 2.0f),
                                                   0.9f, V2D::TextAlign::Center);

                    // sample.pngの状態表示
                    if (hasSampleTexture) {
                        textRenderer.DrawText(batch, font.get(), 
                                             "[OK] sample.png loaded ->",
                                             glm::vec2(750.0f, 150.0f),
                                             glm::vec4(0.5f, 1.0f, 0.5f, 1.0f),
                                             0.8f);
                    } else {
                        textRenderer.DrawText(batch, font.get(), 
                                             "[SKIP] sample.png not found",
                                             glm::vec2(10.0f, 650.0f),
                                             glm::vec4(1.0f, 0.5f, 0.5f, 1.0f),
                                             0.8f);
                    }
                }
                
                // スプライトバッチ終了
                batch->End();
                
                // バッチをレンダリング
                batch->Render(renderer->GetCurrentCommandBuffer());
            }
        );

    } catch (const std::exception& e) {
        std::cerr << "エラー: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
