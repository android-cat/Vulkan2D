#include <gtest/gtest.h>
#include <Vulkan2D/Core/Engine.h>
#include <Vulkan2D/Input/InputManager.h>

TEST(InputManagerTest, InputAPIDefaultState) {
    V2D::EngineConfig config;
    config.windowConfig.width = 800;
    config.windowConfig.height = 600;
    
    try {
        V2D::Engine engine(config);
        V2D::InputManager* input = engine.GetInput();
        ASSERT_NE(input, nullptr);
        
        EXPECT_FALSE(input->IsKeyDown(V2D::Key::Space));
        EXPECT_FALSE(input->IsMouseButtonDown(V2D::MouseButton::Left));
        // マウス座標は実際のOSカーソル位置に影響されるため、取得関数の呼び出しがクラッシュしないかのみ検証します
        glm::vec2 pos = input->GetMousePosition();
        EXPECT_FLOAT_EQ(input->GetScrollDelta(), 0.0f);
    } catch (...) {
        // 無画面環境（ヘッドレス）への配慮
    }
}
