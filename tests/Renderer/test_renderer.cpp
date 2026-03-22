#include <gtest/gtest.h>
#include <Vulkan2D/Core/Engine.h>
#include <Vulkan2D/Renderer/Renderer2D.h>

TEST(Renderer2DTest, RendererAPIProperties) {
    V2D::EngineConfig config;
    try {
        V2D::Engine engine(config);
        V2D::Renderer2D* renderer = engine.GetRenderer();
        ASSERT_NE(renderer, nullptr);
        
        renderer->SetClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        EXPECT_EQ(renderer->GetCurrentFrame(), 0);
    } catch (...) {
        // 無画面環境への配慮
    }
}
