#include <gtest/gtest.h>
#include <Vulkan2D/Graphics/Camera2D.h>
#include <Vulkan2D/Graphics/Sprite.h>

TEST(Camera2DTest, InitializationAndProperties) {
    V2D::Camera2D camera(800.0f, 600.0f);
    EXPECT_FLOAT_EQ(camera.GetPosition().x, 0.0f);
    EXPECT_FLOAT_EQ(camera.GetZoom(), 1.0f);
    EXPECT_FLOAT_EQ(camera.GetRotation(), 0.0f);
}

TEST(Camera2DTest, MovementAndZoomAPI) {
    V2D::Camera2D camera(800.0f, 600.0f);
    camera.SetPosition(glm::vec2(100.0f, 50.0f));
    EXPECT_FLOAT_EQ(camera.GetPosition().x, 100.0f);
    
    camera.Move(glm::vec2(10.0f, -10.0f));
    EXPECT_FLOAT_EQ(camera.GetPosition().x, 110.0f);
    EXPECT_FLOAT_EQ(camera.GetPosition().y, 40.0f);
    
    camera.Zoom(0.5f);
    EXPECT_FLOAT_EQ(camera.GetZoom(), 1.5f); // Assuming zoom adds delta.
    
    camera.Rotate(1.0f);
    EXPECT_FLOAT_EQ(camera.GetRotation(), 1.0f);
}

TEST(SpriteTest, SpriteAPIProperties) {
    V2D::Sprite sprite;
    sprite.SetSize(glm::vec2(50.0f, 50.0f));
    EXPECT_FLOAT_EQ(sprite.GetSize().x, 50.0f);
    
    sprite.SetAlpha(0.5f);
    EXPECT_FLOAT_EQ(sprite.GetColor().a, 0.5f);
    
    sprite.SetColorRGB(255, 0, 0);
    EXPECT_FLOAT_EQ(sprite.GetColor().r, 1.0f);
    EXPECT_FLOAT_EQ(sprite.GetColor().g, 0.0f);
    EXPECT_FLOAT_EQ(sprite.GetColor().b, 0.0f);
}
