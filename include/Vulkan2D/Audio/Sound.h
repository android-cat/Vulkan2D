/**
 * @file Sound.h
 * @brief サウンドクラス
 * 
 * WAV/MP3/OGG/FLACなどの音声ファイルを管理するクラス
 */

#pragma once

#include "Vulkan2D/Audio/SoundData.h"
#include <string>
#include <memory>
#include <cstdint>

namespace V2D {

// 前方宣言
class AudioManager;

/**
 * @brief サウンドデータを保持するクラス
 * 
 * 音声ファイルをロードし、再生に必要なデータを管理する。
 * AudioManagerを通じて再生・停止・音量調整などを行う。
 */
class Sound {
    friend class AudioManager;
public:
    /**
     * @brief コンストラクタ
     * @param filePath 音声ファイルのパス（WAV/MP3/OGG/FLAC対応）
     * @param streaming trueの場合、ストリーミング再生（BGM向け）
     */
    Sound(const std::string& filePath, bool streaming = false);

    /**
     * @brief デストラクタ
     */
    ~Sound();

    // コピー禁止
    Sound(const Sound&) = delete;
    Sound& operator=(const Sound&) = delete;

    // ムーブ可能
    Sound(Sound&& other) noexcept;
    Sound& operator=(Sound&& other) noexcept;

    /**
     * @brief ファイルパスを取得
     * @return ファイルパス
     */
    const std::string& GetFilePath() const { return m_FilePath; }

    /**
     * @brief ストリーミングモードかどうか
     * @return ストリーミングの場合true
     */
    bool IsStreaming() const { return m_Streaming; }

    /**
     * @brief ロードが成功したかどうか
     * @return ロード成功の場合true
     */
    bool IsLoaded() const { return m_Loaded; }

    /**
     * @brief サウンドの長さを取得（秒）
     * @return 長さ（秒）
     */
    float GetDuration() const { return m_Duration; }

    /**
     * @brief サウンドデータへのアクセス（内部使用）
     * @return サウンドデータへのポインタ
     */
    SoundData* GetData() { return m_Data.get(); }
    const SoundData* GetData() const { return m_Data.get(); }

private:
    std::string m_FilePath;
    bool m_Streaming = false;
    bool m_Loaded = false;
    float m_Duration = 0.0f;

    std::unique_ptr<SoundData> m_Data;
};

/**
 * @brief サウンド再生時のハンドル
 * 
 * 再生中のサウンドを制御するためのハンドル。
 * 停止、一時停止、音量調整などに使用する。
 */
class SoundInstance {
    friend class AudioManager;
public:
    SoundInstance() = default;

    /**
     * @brief 有効なインスタンスかどうか
     * @return 有効な場合true
     */
    bool IsValid() const { return m_Id != 0; }

    /**
     * @brief インスタンスIDを取得
     * @return インスタンスID
     */
    uint64_t GetId() const { return m_Id; }

private:
    explicit SoundInstance(uint64_t id) : m_Id(id) {}
    uint64_t m_Id = 0;
};

} // namespace V2D
