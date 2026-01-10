/**
 * @file SoundData.h
 * @brief サウンドデータの内部構造体
 * 
 * Sound と AudioManager で共有されるデータ構造
 */

#pragma once

#include <vector>
#include <cstdint>

// miniaudioの型を前方宣言するため
struct ma_decoder;

namespace V2D {

/**
 * @brief サウンドデータの内部構造体
 * 
 * デコードされたPCMデータとフォーマット情報を保持する
 */
struct SoundData {
    // miniaudioのデコーダー（ストリーミング用）
    ma_decoder* decoder = nullptr;
    bool decoderInitialized = false;
    
    // 非ストリーミング用のデコード済みデータ
    std::vector<float> pcmData;
    uint32_t channels = 0;
    uint32_t sampleRate = 0;
    uint64_t totalFrames = 0;
};

} // namespace V2D
