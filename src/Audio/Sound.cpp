/**
 * @file Sound.cpp
 * @brief サウンドクラスの実装
 * 
 * miniaudioを使用したサウンドファイルのロードと管理
 */

#include "Vulkan2D/Audio/Sound.h"

// miniaudioの実装（このファイルでのみ定義）
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <stdexcept>
#include <filesystem>

namespace V2D {

Sound::Sound(const std::string& filePath, bool streaming)
    : m_FilePath(filePath)
    , m_Streaming(streaming)
    , m_Data(std::make_unique<SoundData>())
{
    // ファイルの存在確認
    if (!std::filesystem::exists(filePath)) {
        throw std::runtime_error("Sound file not found: " + filePath);
    }

    // デコーダーを動的に確保
    m_Data->decoder = new ma_decoder();

    // デコーダー設定
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 44100);
    
    // デコーダーを初期化
    ma_result result = ma_decoder_init_file(filePath.c_str(), &decoderConfig, m_Data->decoder);
    if (result != MA_SUCCESS) {
        delete m_Data->decoder;
        m_Data->decoder = nullptr;
        throw std::runtime_error("Failed to initialize audio decoder for: " + filePath);
    }
    m_Data->decoderInitialized = true;

    // フォーマット情報を取得
    m_Data->channels = m_Data->decoder->outputChannels;
    m_Data->sampleRate = m_Data->decoder->outputSampleRate;

    // 総フレーム数を取得
    ma_uint64 totalFrames = 0;
    result = ma_decoder_get_length_in_pcm_frames(m_Data->decoder, &totalFrames);
    if (result == MA_SUCCESS && totalFrames > 0) {
        m_Data->totalFrames = totalFrames;
        m_Duration = static_cast<float>(totalFrames) / static_cast<float>(m_Data->sampleRate);
    }

    // 非ストリーミングモードの場合、全データをメモリにロード
    if (!streaming && m_Data->totalFrames > 0) {
        m_Data->pcmData.resize(m_Data->totalFrames * m_Data->channels);
        
        ma_uint64 framesRead = 0;
        result = ma_decoder_read_pcm_frames(m_Data->decoder, m_Data->pcmData.data(), m_Data->totalFrames, &framesRead);
        if (result != MA_SUCCESS) {
            throw std::runtime_error("Failed to decode audio file: " + filePath);
        }
        
        // 実際に読み込んだフレーム数で調整
        m_Data->pcmData.resize(framesRead * m_Data->channels);
        m_Data->totalFrames = framesRead;
        
        // デコーダーは不要になったので解放（データはメモリに保持）
        ma_decoder_uninit(m_Data->decoder);
        delete m_Data->decoder;
        m_Data->decoder = nullptr;
        m_Data->decoderInitialized = false;
    }

    m_Loaded = true;
}

Sound::~Sound() {
    if (m_Data && m_Data->decoderInitialized && m_Data->decoder) {
        ma_decoder_uninit(m_Data->decoder);
        delete m_Data->decoder;
    }
}

Sound::Sound(Sound&& other) noexcept
    : m_FilePath(std::move(other.m_FilePath))
    , m_Streaming(other.m_Streaming)
    , m_Loaded(other.m_Loaded)
    , m_Duration(other.m_Duration)
    , m_Data(std::move(other.m_Data))
{
    other.m_Loaded = false;
}

Sound& Sound::operator=(Sound&& other) noexcept {
    if (this != &other) {
        if (m_Data && m_Data->decoderInitialized && m_Data->decoder) {
            ma_decoder_uninit(m_Data->decoder);
            delete m_Data->decoder;
        }
        
        m_FilePath = std::move(other.m_FilePath);
        m_Streaming = other.m_Streaming;
        m_Loaded = other.m_Loaded;
        m_Duration = other.m_Duration;
        m_Data = std::move(other.m_Data);
        
        other.m_Loaded = false;
    }
    return *this;
}

} // namespace V2D
