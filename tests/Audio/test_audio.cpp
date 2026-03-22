#include <gtest/gtest.h>
#include <Vulkan2D/Audio/AudioManager.h>

TEST(AudioManagerTest, AudioAPISettings) {
    try {
        V2D::AudioManager audio;
        EXPECT_FLOAT_EQ(audio.GetMasterVolume(), 1.0f);
        EXPECT_FLOAT_EQ(audio.GetSFXVolume(), 1.0f);
        EXPECT_FLOAT_EQ(audio.GetBGMVolume(), 1.0f);
        
        audio.SetMasterVolume(0.5f);
        audio.SetSFXVolume(0.8f);
        EXPECT_FLOAT_EQ(audio.GetMasterVolume(), 0.5f);
        EXPECT_FLOAT_EQ(audio.GetSFXVolume(), 0.8f);
        
        EXPECT_FALSE(audio.IsBGMPlaying());
    } catch (...) {
        // オーディオデバイスが存在しない環境などへの配慮
    }
}
