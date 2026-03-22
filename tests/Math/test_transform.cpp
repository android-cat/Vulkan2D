#include <gtest/gtest.h>
#include <Vulkan2D/Math/Transform2D.h>

TEST(Transform2DTest, DefaultInitialization) {
    V2D::Transform2D t;
    EXPECT_FLOAT_EQ(t.position.x, 0.0f);
    EXPECT_FLOAT_EQ(t.position.y, 0.0f);
    EXPECT_FLOAT_EQ(t.scale.x, 1.0f);
    EXPECT_FLOAT_EQ(t.scale.y, 1.0f);
    EXPECT_FLOAT_EQ(t.rotation, 0.0f);
}

TEST(Transform2DTest, Translate) {
    V2D::Transform2D t;
    t.Translate(glm::vec2(10.0f, -5.0f));
    EXPECT_FLOAT_EQ(t.position.x, 10.0f);
    EXPECT_FLOAT_EQ(t.position.y, -5.0f);
}

TEST(Transform2DTest, SetScale) {
    V2D::Transform2D t;
    t.SetScale(2.5f);
    EXPECT_FLOAT_EQ(t.scale.x, 2.5f);
    EXPECT_FLOAT_EQ(t.scale.y, 2.5f);
}

TEST(Transform2DTest, Rotate) {
    V2D::Transform2D t;
    t.Rotate(3.14159f); // Rotate by Pi
    EXPECT_FLOAT_EQ(t.rotation, 3.14159f);
}
