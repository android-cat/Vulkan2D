#include <gtest/gtest.h>
#include <Vulkan2D/Core/Engine.h>

TEST(EngineTest, Initialization) {
    V2D::EngineConfig config;
    // WindowConfigの設定 (一般的なプロパティであると仮定)
    config.windowConfig.width = 800;
    config.windowConfig.height = 600;
    config.windowConfig.title = "Vulkan2D Test Engine";
    config.enableValidation = true;

    try {
        V2D::Engine engine(config);
        EXPECT_NE(engine.GetWindow(), nullptr);
        EXPECT_NE(engine.GetVulkanContext(), nullptr);
    } catch (const std::exception& e) {
        FAIL() << "Engine initialization threw an exception: " << e.what();
    }
}
